#include <cstdio> // printf
#include <cstdlib> // abs
#include <cmath> // sqrt
#include <cassert> // assert
#include <algorithm> // max

#include "single_atom.hxx"

#include "radial_grid.hxx" // create_exponential_radial_grid, destroy_radial_grid
#include "radial_eigensolver.hxx" // shooting_method
#include "angular_grid.hxx" // transform, Lebedev_grid_size
#include "radial_integrator.hxx" // integrate_outwards
#include "inline_tools.hxx" // align<nbits>
#include "sho_tools.hxx" // lnm_index
#include "solid_harmonics.hxx" // lm_index, Y00, Y00inv
#include "atom_core.hxx" // dot_product, initial_density
using namespace atom_core;

#include "quantum_numbers.h" // enn_QN_t, ell_QN_t, emm_QN_t, emm_Degenerate, spin_QN_t, spin_Degenerate
#include "output_units.h" // eV, _eV

// #define FULL_DEBUG
#define DEBUG

#ifdef  DEBUG
    #include "debug_output.hxx" // dump_to_file
#endif

#ifdef FULL_DEBUG
    #define full_debug(print) print 
#else
    #define full_debug(print)
#endif

#ifdef DEBUG
    #define debug(print) print 
#else
    #define debug(print)
#endif

  int constexpr TRU=0, SMT=1, TRU_AND_SMT=2;
  int constexpr CORE=0, VALENCE=1;
  int constexpr ELLMAX=7;
  char const *const ellchar = "spdfghijkl";
  
//   int constexpr NUMCORESTATES=20; // 20 are ok, 32 are enough if spin-orbit-interaction is on
//   int constexpr NUMVALENCESTATES=(ELLMAX*(ELLMAX + 4) + 4)/4;

  // ToDo: write a class with constructors and destructors to handle the memory for *wave
  template<int Pseudo> // Pseudo=1: core states, Pseudo=2: valence states
  struct energy_level {
      double* wave[Pseudo]; // for valence states points to the true and smooth partial waves
      double energy; // energy level in Hartree atomic units
      float occupation; // occupation number
      enn_QN_t enn; // main quantum_number
      ell_QN_t ell; // angular momentum quantum_number
      emm_QN_t emm; // usually emm == emm_Degenerate
      spin_QN_t spin; // usually spin == spin_Degenerate
      enn_QN_t nrn[Pseudo]; // number of radial nodes
  };

  typedef struct energy_level<1+CORE>       core_level_t;
  typedef struct energy_level<1+VALENCE> valence_level_t;
  
  class LiveAtom {
  public:
//    double* mem; // memory to which all radial function pointers and matrices point
      
      // general config
      int32_t id; // global atom identifyer
      float Z; // number of nucleons in the core
      radial_grid_t* rg[TRU_AND_SMT]; // radial grid descriptor for the true and smooth grid, SMT may point to TRU
      ell_QN_t ellmax; // limit ell for full_potential and full_density
      double r_cut; // classical augmentation radius for potential and core density
      float r_match; // radius for matching of true and smooth partial wave, usually 6--9*sigma
      ell_QN_t numax; // limit of the SHO projector quantum numbers
      uint8_t nn[1+ELLMAX]; // number of projectors and partial waves used in each ell-channel
      ell_QN_t ellmax_compensator; // limit ell for the charge deficit compensators
      double sigma_compensator; // Gaussian spread for the charge deficit compensators
      double* rho_compensator; // coefficients for the charge deficit compensators, (1+ellmax_compensator)^2
      double* aug_density; // augmented density, core + valence + compensation, (1+ellmax)^2 radial functions
      int matrix_stride; // stride for the matrices hamiltonian and overlap, must be >= ((1+numax)*(2+numax)*(3+numax))/6
      int8_t ncorestates; // for emm-Degenerate representations, 20 (or 32 with spin-oribit) core states are maximum
      int8_t nvalencestates; // for emm-Degenerate (numax*(numax + 4) + 4)/4 is a good choice;
      int8_t nspins; // 1 or 2 or 4

      // spin-resolved members of live_atom
      core_level_t* core_state;
      valence_level_t* valence_state;
      double* core_density[TRU_AND_SMT]; // spherical core density
      double* full_density[TRU_AND_SMT]; // total density, core + valence, (1+ellmax)^2 radial functions
      double* full_potential[TRU_AND_SMT]; // (1+ellmax)^2 radial functions
      double* potential[TRU_AND_SMT]; // spherical potential r*V(r), no Y00 factor
      double* bar_potential; // PAW potential shape correction
      double  sigma, sigma_inv; // spread of the SHO projectors and its inverse
      double *hamiltonian, *overlap; // matrices [nSHO*nSHO]
      double (*kinetic_energy)[TRU_AND_SMT]; // matrix [nln*nln][TRU_AND_SMT]
      double (*charge_deficit)[TRU_AND_SMT]; // matrix [(1 + ellmax_compensator)*nln*nln][TRU_AND_SMT]

      bool gaunt_init;
      std::vector<gaunt_entry_t> gaunt;
      
      
  public:
    LiveAtom(double const Z_nucleons) {; // constructor
        int constexpr echo = 1;
        id = -1; // unset
        Z = Z_nucleons; // convert to float
        rg[TRU] = radial_grid::create_exponential_radial_grid(250*std::sqrt(Z + 9.));
//      rg[SMT] = rg[TRU]; // flat copy, alternatively, we could create a radial grid descriptor which has less points at the origin:
        rg[SMT] = radial_grid::create_pseudo_radial_grid(*rg[TRU]);
        int const nrt = align<2>(rg[TRU]->n),
                  nrs = align<2>(rg[SMT]->n);
        if (echo > 0) printf("# padded radial grid numbers are %d and %d\n", nrt, nrs);
        numax = 3; // up to f-projectors
        ellmax = 0; // should be 2*numax;
        ellmax_compensator = 0;
        r_cut = 2.0; // Bohr
        sigma = 0.5; // Bohr
        sigma_compensator = 1.0; // Bohr
        r_match = 9*sigma;
        if (echo > 0) printf("# numbers of projectors ");
        for(int ell = 0; ell <= ELLMAX; ++ell) {
            nn[ell] = std::max(0, (numax + 2 - ell)/2);
            if (echo > 0) printf(" %d", nn[ell]);
        } // ell
        if (echo > 0) printf("\n");
        
        gaunt_init = false;

        int enn_core_max[10] = {0,1,2,3,4,5,6,7,8,9};
        ncorestates = 20;
        core_state = new core_level_t[ncorestates];
        {   int ics = 0, jcs = -1; float ne = Z;
            for(int m = 0; m < 8; ++m) { // auxiliary number
                int enn = (m + 1)/2;
                for(int ell = m/2; ell >= 0; --ell) { // angular momentum character
                    ++enn; // principal quantum number
                    for(int jj = 2*ell; jj >= 2*ell; jj -= 2) {
                        float const max_occ = 2*(jj + 1);
                        float const occ = std::min(std::max(0.f, ne), 2.f*(jj + 1));
                        core_state[ics].energy = -.5*(Z/enn)*(Z/enn); // hydrogen like energy levels
                        // warning: this memory is not freed
                        core_state[ics].wave[TRU] = new double[nrt]; // get memory for the (true) radial function
                        core_state[ics].nrn[TRU] = enn - ell; // number of radial nodes
                        core_state[ics].enn = enn;
                        core_state[ics].ell = ell;
                        core_state[ics].emm = emm_Degenerate;
                        core_state[ics].spin = spin_Degenerate;
                        core_state[ics].occupation = occ;
                        if (max_occ == occ) {
                            if (echo > 0) printf("# core    %2d%c%6.1f \n", enn, ellchar[ell], occ);
                            enn_core_max[ell] = std::max(enn_core_max[ell], enn); 
                        }
                        if (occ > 0) jcs = ics;
                        ne -= max_occ;
                        ++ics;
                    } // jj
                } // ell
            } // m
            ncorestates = jcs + 1;
        } // core states
        
        nvalencestates = (numax*(numax + 4) + 4)/4;
        valence_state = new valence_level_t[nvalencestates];
        {   int iln = 0;
            for(int ell = 0; ell <= numax; ++ell) {
                for(int nrn = 0; nrn < nn[ell]; ++nrn) {
                    int const enn = nrn + enn_core_max[ell] + 1;
                    if (echo > 0) printf("# valence %2d%c\n", enn, ellchar[ell]);
                    valence_state[iln].energy = -.5*(Z/enn)*(Z/enn); // hydrogen like energy levels
                    // warning: this memory is not freed
                    valence_state[iln].wave[TRU] = new double[nrt]; // get memory for the true radial function
                    valence_state[iln].wave[SMT] = new double[nrs]; // get memory for the smooth radial function
                    valence_state[iln].nrn[TRU] = enn - ell; // number of radial nodes
                    valence_state[iln].nrn[SMT] = nrn;
                    valence_state[iln].occupation = 0;
                    valence_state[iln].enn = enn;
                    valence_state[iln].ell = ell;
                    valence_state[iln].emm = emm_Degenerate;
                    valence_state[iln].spin = spin_Degenerate;
                    ++iln;
                } // nrn
            } // ell
        } // valence states
        
        int const nln = nvalencestates;
        int const nlm = (1 + ellmax)*(1 + ellmax);
        for(int ts = TRU; ts < TRU_AND_SMT; ts += (SMT - TRU)) {
            int const nr = (TRU == ts)? nrt : nrs;
            core_density[ts]   = new double[nr]; // get memory
            potential[ts]      = new double[nr]; // get memory
            full_density[ts]   = new double[nlm*nr]; // get memory
            full_potential[ts] = new double[nlm*nr]; // get memory
        } // true and smooth
        aug_density     = new double[nlm*nrs]; // get memory
        int const nlm_cmp = (1 + ellmax_compensator)*(1 + ellmax_compensator);
        rho_compensator = new double[nlm_cmp]; // get memory
        charge_deficit  = new double[(1 + ellmax_compensator)*nln*nln][TRU_AND_SMT]; // get memory
        bar_potential   = new double[nrs]; // get memory
        kinetic_energy  = new double[nln*nln][TRU_AND_SMT]; // get memory

        int const nSHO = sho_tools::nSHO(numax);
        matrix_stride = align<2>(nSHO); // 2^<2> doubles = 32 Byte alignment
        hamiltonian = new double[nSHO*matrix_stride]; // get memory
        overlap     = new double[nSHO*matrix_stride]; // get memory

//      for(int ir = 0; ir < nrt; ++ir) { potential[TRU][ir] = -Z; } // unscreened hydrogen-type potential
        initial_density(core_density[TRU], *rg[TRU], Z, 0.0);
        for(int scf = 0; scf < 133; ++scf) {
            rad_pot(potential[TRU], *rg[TRU], core_density[TRU], Z);
            update(9);
            printf("\n");
        } // self-consistency iterations

    };
      
    ~LiveAtom() { // destructor
        // warning: cleanup does not cover all allocations
//      if (rg[SMT] != rg[TRU]) radial_grid::destroy_radial_grid(rg[SMT]); // gives memory corruption
        radial_grid::destroy_radial_grid(rg[TRU]);
        delete[] core_state;
        delete[] valence_state;
    };
    
    void update_core_states(float const mixing, int echo=0) {
        int const nr = rg[TRU]->n;
        auto r2rho = new double[nr];
        auto new_r2core_density = std::vector<double>(nr, 0.0);
        for(int ics = 0; ics < ncorestates; ++ics) {
            auto &cs = core_state[ics];
            int constexpr SRA = 1;
            radial_eigensolver::shooting_method(SRA, *rg[TRU], potential[TRU], cs.enn, cs.ell, cs.energy, cs.wave[TRU], r2rho);
            if (echo > 0) printf("# core    %2d%c%6.1f E=%16.6f %s\n", cs.enn, ellchar[cs.ell], cs.occupation, cs.energy*eV,_eV);
            auto const norm = dot_product(nr, r2rho, rg[TRU]->dr);
            auto const scal = (norm > 0)? cs.occupation/norm : 0;
            for(int ir = 0; ir < nr; ++ir) {
                new_r2core_density[ir] += scal*r2rho[ir];
            } // ir
        } // ics
        double core_density_change = 0, core_nuclear_energy = 0;
        for(int ir = 0; ir < nr; ++ir) {
            auto const new_rho = new_r2core_density[ir]*(rg[TRU]->rinv[ir]*rg[TRU]->rinv[ir]); // *r^{-2}
            core_density_change += std::abs(new_rho - core_density[TRU][ir])*rg[TRU]->r2dr[ir];
            core_nuclear_energy +=         (new_rho - core_density[TRU][ir])*rg[TRU]->rdr[ir];
            core_density[TRU][ir] = mixing*new_rho + (1. - mixing)*core_density[TRU][ir];
        } // ir
        core_nuclear_energy *= -Z;
        if (echo > 0) printf("# core density change %g e, energy change %g %s\n", core_density_change, core_nuclear_energy*eV,_eV);
    } // update

    void update_valence_states(int echo=0) {
//      auto small_component = new double[rg[TRU]->n];
        for(int iln = 0; iln < nvalencestates; ++iln) {
            auto &vs = valence_state[iln];
            int constexpr SRA = 1;
            vs.energy = -.25; // random number
//          radial_integrator::integrate_outwards<SRA>(*rg[TRU], potential[TRU], vs.ell, vs.energy, vs.wave[TRU], small_component);
            radial_eigensolver::shooting_method(SRA, *rg[TRU], potential[TRU], vs.enn, vs.ell, vs.energy, vs.wave[TRU]);
            if (echo > 0) printf("# valence %2d%c%6.1f E=%16.6f %s\n", vs.enn, ellchar[vs.ell], vs.occupation, vs.energy*eV,_eV);
        } // iln
    } // update

    void update_charge_deficit(int echo=9) {
        int const nln = nvalencestates;
        for(int ts = TRU; ts < TRU_AND_SMT; ts += (SMT - TRU)) {
            int const nr = rg[ts]->n;
            double rl[nr], wave_rlr2dr[nr];
            for(int ir = 0; ir < nr; ++ir) {
                rl[ir] = 1; // init as r^0
            } // ir
            for(int ell = 0; ell <= ellmax_compensator; ++ell) { // loop-carried dependency on rl, run forward, run serial!
                for(int iln = 0; iln < nln; ++iln) {
                    for(int ir = 0; ir < nr; ++ir) {
                        wave_rlr2dr[ir] = valence_state[iln].wave[ts][ir] * rl[ir] * rg[ts]->r2dr[ir];
                    } // ir
                    for(int jln = 0; jln < nln; ++jln) {
                        charge_deficit[(ell*nln + iln)*nln + jln][ts] = dot_product(nr, wave_rlr2dr, valence_state[jln].wave[ts]);
                    } // jln
                } // iln
                for(int ir = 0; ir < nr; ++ir) {
                    rl[ir] *= rg[ts]->r[ir]; // create r^(ell + 1) for the next iteration
                } // ir
            } // ell
        } // ts
    } // update
    
    void get_valence_mapping(int16_t ln_index_list[], int const nlmn, int const nln, 
                             int16_t lmn_begin[], int16_t lmn_end[], int const mlm, 
                             int const echo=0) {
        for(int lm = 0; lm < mlm; ++lm) lmn_begin[lm] = -1;
        int ilmn = 0;
        for(int16_t ell = 0; ell <= numax; ++ell) {
            int iln_enn[nn[ell]]; // create a table of iln-indices
            for(int iln = 0; iln < nln; ++iln) {
                if (ell == valence_state[iln].ell) {
                    int const nrn = valence_state[iln].nrn[SMT];
                    iln_enn[nrn] = iln;
                } // ell matches
            } // iln
            for(int16_t emm = -ell; emm <= ell; ++emm) {
                for(int16_t nrn = 0; nrn < nn[ell]; ++nrn) {
                    ln_index_list[ilmn] = iln_enn[nrn]; // valence state index
                    int const lm = solid_harmonics::lm_index(ell, emm);
                    if (lmn_begin[lm] < 0) lmn_begin[lm] = ilmn; // store the first index of this lm
                                             lmn_end[lm] = ilmn + 1; // store the last index of this lm
                    ++ilmn;
                } // nrn
            } // emm
        } // ell
        assert(nlmn == ilmn);
        
        if (echo > 3) {
            printf("# ln_index_list ");
            for(int i = 0; i < nlmn; ++i) {
                printf("%3d", ln_index_list[i]); 
            }   printf("\n");
            printf("# lmn_begin--lmn_end "); 
            for(int i = 0; i < mlm; ++i) {
//              if (lmn_begin[i] == lmn_end[i] - 1) { 
                if (0) { 
                    printf(" %d", lmn_begin[i]); 
                } else {
                    printf(" %d--%d", lmn_begin[i], lmn_end[i] - 1); 
                }
            }   printf("\n");
        } // echo
        
    } // get_valence_mapping
    
    void get_rho_tensor(double rho_tensor[], double const density_matrix[], int const echo=9) {
        int const nSHO = sho_tools::nSHO(numax);
        int const stride = nSHO;
        assert(stride >= nSHO);
        
        if (!gaunt_init) gaunt_init = (0 == angular_grid::create_numerical_Gaunt<6>(&gaunt));
        
        sho_tools::all_tests();
        
        int const lmax = std::max(ellmax, ellmax_compensator);
        int const nlm = (1 + lmax)*(1 + lmax);
        int const mlm = (1 + numax)*(1 + numax);
        int const nln = nvalencestates;
        auto const radial_density_matrix = density_matrix; // flat copy, cheat, ToDo: transform from the Cartesian rep to radial using sho_unitary
        // ToDo:
        //   transform the density_matrix[iSHO*stride + jSHO]
        //   into a radial_density_matrix[ilmn*stride + jlmn]
        //   using the unitary transform from left and right
        //   Then, contract with the Gaunt tensor over m_1 and m_2
        //   rho_tensor[(lm*nln + iln)*nln + jln] = 
        //     G_{lm l_1m_1 l_2m_2} * density_matrix[il_1m_1n_1*stride + jl_2m_2n_2]

        int const nlmn = nSHO;
        int16_t ln_index_list[nlmn], lmn_begin[mlm], lmn_end[mlm];
        get_valence_mapping(ln_index_list, nlmn, nln, lmn_begin, lmn_end, mlm);

        for(int lmij = 0; lmij < nlm*nln*nln; ++lmij) rho_tensor[lmij] = 0; // clear
        for(auto gnt : gaunt) {
            int const lm = gnt.lm, lm1 = gnt.lm1, lm2 = gnt.lm2; auto const G = gnt.G;
            if (lm < nlm && lm1 < mlm && lm2 < mlm) {
                for(int ilmn = lmn_begin[lm1]; ilmn < lmn_end[lm1]; ++ilmn) {
                    int const iln = ln_index_list[ilmn];
                    for(int jlmn = lmn_begin[lm2]; jlmn < lmn_end[lm2]; ++jlmn) {
                        int const jln = ln_index_list[jlmn];
                        rho_tensor[(lm*nln + iln)*nln + jln] += G * radial_density_matrix[ilmn*stride + jlmn];
                    } // jlmn
                } // ilmn
            } // limits
        } // gnt
        
    } // get
    
    
    template<int ADD0_or_PROJECT1>
    void add_or_project_compensators(double out[], int const lmax, radial_grid_t const *rg, double const in[], int echo=0) {
        int const nr = rg->n, mr = align<2>(nr);
        auto const sig2inv = -0.5/(sigma_compensator*sigma_compensator);
        if (echo > 0) printf("# sigma = %g\n", sigma_compensator);
        double rl[nr], rlgauss[nr];
        for(int ell = 0; ell <= lmax; ++ell) { // serial!
            double norm = 0;
            for(int ir = 0; ir < nr; ++ir) {
                auto const r = rg->r[ir];
                if (0 == ell) {
                    rl[ir] = 1; // start with r^0
                    rlgauss[ir] = std::exp(sig2inv*r*r);
                } else {
                    rl[ir] *= r; // construct r^ell
                    rlgauss[ir] *= r; // construct r^ell*gaussian
                }
                norm += rlgauss[ir] * rl[ir] * rg->r2dr[ir];
                if (echo > 6) printf("# ell=%d norm=%g ir=%d rlgauss=%g rl=%g r2dr=%g\n", ell, norm, ir, rlgauss[ir], rl[ir], rg->r2dr[ir]);
            } // ir
            if (echo > 1) printf("# ell=%d norm=%g nr=%d\n", ell, norm, nr);
            assert(norm > 0);
            auto const scal = 1./norm;
            for(int emm = -ell; emm <= ell; ++emm) {
                int const lm = solid_harmonics::lm_index(ell, emm);
                if (0 == ADD0_or_PROJECT1) {
                    auto const rho_lm_scal = in[lm] * scal;
                    for(int ir = 0; ir < nr; ++ir) {
                        out[lm*mr + ir] += rho_lm_scal * rlgauss[ir];
                    } // ir
                } else {
                    double dot = 0;
                    for(int ir = 0; ir < nr; ++ir) {
                        dot += in[lm*mr + ir] * rlgauss[ir];
                    } // ir
                    out[lm] = dot * scal;
                } // add or project
            } // emm
        } // ell
  
    } // compensators

    
    void electrostatic(double pot[], int const lmax, radial_grid_t const *rg, double const rho[]) {
        int const nr = rg->n, mr = align<2>(nr);
        double rl[nr], rml1[nr], coeff[2*lmax + 1];
        for(int ell = 0; ell <= lmax; ++ell) { // serial!
            for(int ir = 0; ir < nr; ++ir) {
                auto const r = rg->r[ir];
                auto const ri = rg->rinv[ir];
                if (0 == ell) {
                    rl[ir] = 1; // start with r^0
                    rml1[ir] = ri; // start with r^{-1}
                } else {
                    rl[ir] *= r; // construct r^ell
                    rml1[ir] *= ri; // construct r^(-1-ell)
                }
            } // ir
            for(int emm = -ell; emm <= ell; ++emm) {
                int const lm = solid_harmonics::lm_index(ell, emm);
                
                for(int ir = 0; ir < nr; ++ir) {
                    pot[lm*mr + ir] += coeff[lm] * rml1[ir];
                } // ir
            } // emm
        } // ell
  
    } // compensators
    
    
    void update_full_density(double const rho_tensor[]) { // density tensor rho_{lm iln jln}
        int const nlm = (1 + ellmax)*(1 + ellmax);
        int const nln = nvalencestates;
        
        for(int ts = TRU; ts < TRU_AND_SMT; ts += (SMT - TRU)) {
            int const nr = rg[ts]->n, mr = align<2>(nr);
            // full_density[ts][nlm*mr]; // memory layout
            for(int lm = 0; lm < nlm; ++lm) {
                if (0 == lm) {
                    for(int ir = 0; ir < mr; ++ir) {
                        full_density[ts][lm*mr + ir] = core_density[ts][ir]; // needs scaling with Y00 here?
                    } // ir
                } else {
                    for(int ir = 0; ir < mr; ++ir) {
                        full_density[ts][lm*mr + ir] = 0; // init clear
                    } // ir
                }
                for(int iln = 0; iln < nln; ++iln) {
                    auto const wave_i = valence_state[iln].wave[ts];
                    for(int jln = 0; jln < nln; ++jln) {
                        auto const wave_j = valence_state[jln].wave[ts];
                        double const rho_ij = rho_tensor[(lm*nln + iln)*nln + jln];
                        for(int ir = 0; ir < nr; ++ir) {
                            full_density[ts][lm*mr + ir] += rho_ij * wave_i[ir] * wave_j[ir];
                        } // ir
                    } // jln
                } // iln
            } // lm
        } // true and smooth

        int const nlm_cmp = (1 + ellmax_compensator)*(1 + ellmax_compensator);
        for(int ell = 0; ell <= ellmax_compensator; ++ell) {
            for(int emm = -ell; emm <= ell; ++emm) {
                int const lm = solid_harmonics::lm_index(ell, emm);
                double rho_lm = 0;
                for(int iln = 0; iln < nln; ++iln) {
                    for(int jln = 0; jln < nln; ++jln) {
                        double const rho_ij = rho_tensor[(lm*nln + iln)*nln + jln];
                        rho_lm += rho_ij * ( charge_deficit[(ell*nln + iln)*nln + jln][TRU]
                                           - charge_deficit[(ell*nln + iln)*nln + jln][SMT] );
                    } // jln
                } // iln
                rho_compensator[lm] = rho_lm;
            } // emm
        } // ell
        
        rho_compensator[0] -= Z * solid_harmonics::Y00; // account for the protons in the nucleus

        // construct the augmented density
        {   int const nr = rg[SMT]->n, mr = align<2>(nr);
            for(int ij = 0; ij < nlm*mr; ++ij) {
                aug_density[ij] = full_density[SMT][ij]; // need spin summation?
            } // ij
            for(int ij = nlm*mr; ij < nlm_cmp*mr; ++ij) {
                aug_density[ij] = 0; // in case ellmax_compensator > ellmax
            } // ij
            add_or_project_compensators<0>(aug_density, ellmax_compensator, rg[SMT], rho_compensator);
        } // scope

    } // update

    
    void update_full_potential(double const q_lm[]) {
//      int const nlm = (1 + ellmax)*(1 + ellmax);
        int const npt = angular_grid::Lebedev_grid_size(ellmax);
        for(int ts = TRU; ts < TRU_AND_SMT; ts += (SMT - TRU)) {
            int const nr = rg[ts]->n, mr = align<2>(nr);
            // full_potential[ts][nlm*mr]; // memory layout
            auto on_grid = new double[npt*mr];
            // transform the lm-index into real-space 
            // using an angular grid quadrature, e.g. Lebendev-Laikov grids
            angular_grid::transform(on_grid, full_density[ts], mr, ellmax, false);
            // envoke the exchange-correlation potential (acts in place)
//          printf("# envoke the exchange-correlation on angular grid\n");
            // transform back to lm-index
            angular_grid::transform(full_potential[ts], on_grid, mr, ellmax, true);
            
            { // scope add electrostatic boundary elements q_lm*r^\ell
                double rl[nr];
                for(int ir = 0; ir < nr; ++ir) {
                    rl[ir] = 1; // init as r^0
                } // ir
                for(int ell = 0; ell <= ellmax_compensator; ++ell) { // loop-carried dependency on rl, run forward, run serial!
                    for(int emm = -ell; emm <= ell; ++emm) {
                        int const lm = solid_harmonics::lm_index(ell, emm);
                        for(int ir = 0; ir < nr; ++ir) {
                            full_potential[ts][lm*mr + ir] += q_lm[lm] * rl[ir];
                        } // ir
                    } // emm
                    for(int ir = 0; ir < nr; ++ir) {
                        rl[ir] *= rg[ts]->r[ir]; // prepare r^(ell + 1) for the next iteration
                    } // ir
                } // ell
            } // scope
            
        } // true and smooth
        // add zero potential for SMT==ts and 0==lm
        auto const scale = solid_harmonics::Y00; // *Y00 or *Y00inv? needed?
        for(int ir = 0; ir < rg[SMT]->n; ++ir) {
            full_potential[SMT][0 + ir] += bar_potential[ir] * scale;
        } // ir

    } // update
    
    void update_matrix_elements(int const echo=9) {
        int const nlm = (1 + ellmax)*(1 + ellmax);
        int const mlm = (1 + numax)*(1 + numax);
        int const nln = nvalencestates;
        int const nSHO = sho_tools::nSHO(numax);
        int const nlmn = nSHO;
        
        if (!gaunt_init) gaunt_init = (0 == angular_grid::create_numerical_Gaunt<6>(&gaunt));

        int16_t ln_index_list[nlmn], lmn_begin[mlm], lmn_end[mlm];
        get_valence_mapping(ln_index_list, nlmn, nln, lmn_begin, lmn_end, mlm);
        
        // first generate the matrix elemnts in the valence basis
        //    overlap[iln*nln + jln] and potential[(lm*nln + iln)*nln + jln]
        // then, generate the matrix elements in the radial representation
        //    overlap[ilmn*nlmn + jlmn] and hamiltonian[ilmn*nlmn + jlmn]
        //    where hamiltonian[ilmn*nlmn + jlmn] = kinetic_energy_deficit_{iln jln} 
        //            + sum_lm Gaunt_{lm ilm jlm} * potential[(lm*nln + iln)*nln + jln]
        // then, transform the matrix elements into the Cartesian representation using sho_unitary
        //    overlap[iSHO*nSHO + jSHO] and hamiltonian[iSHO*nSHO + jSHO]
        
        double overlap_ln[nln*nln][TRU_AND_SMT], potential_ln[nlm*nln*nln][TRU_AND_SMT];
        for(int ts = TRU; ts < TRU_AND_SMT; ts += (SMT - TRU)) {
            int const nr = rg[ts]->n, mr = align<2>(nr);
            double wave_pot_r2dr[mr];
            {   int const lm = 0, ell = 0; double const Y00 = solid_harmonics::Y00; // spherical contributions
                for(int iln = 0; iln < nln; ++iln) {
                    for(int ir = 0; ir < nr; ++ir) { // spherical potential[ts][nr] stored as r*V(r), no Y00 factor
                        wave_pot_r2dr[ir] = valence_state[iln].wave[ts][ir] * potential[ts][ir] * rg[ts]->rdr[ir];
                    } // ir
                    for(int jln = 0; jln < nln; ++jln) {
                        potential_ln[(lm*nln + iln)*nln + jln][ts] = dot_product(nr, wave_pot_r2dr, valence_state[jln].wave[ts])*Y00;
                        overlap_ln[iln*nln + jln][ts] = charge_deficit[(ell*nln + iln)*nln + jln][ts];
                    } // jln
                } // iln
            } // 0 == lm
            // non-spherical constributions
            for(int lm = 1; lm < nlm; ++lm) {
                for(int iln = 0; iln < nln; ++iln) {
                    for(int ir = 0; ir < nr; ++ir) { // data layout full_potential[ts][nlm*mr]
                        wave_pot_r2dr[ir] = valence_state[iln].wave[ts][ir] * full_potential[ts][lm*mr + ir] * rg[ts]->r2dr[ir];
                    } // ir
                    for(int jln = 0; jln < nln; ++jln) {
                        potential_ln[(lm*nln + iln)*nln + jln][ts] = dot_product(nr, wave_pot_r2dr, valence_state[jln].wave[ts]);
                    } // jln
                } // iln
            } // lm

        } // ts: true and smooth

        double overlap_lmn[nlmn*nlmn], hamiltonian_lmn[nlmn*nlmn];
        for(int ij = 0; ij < nlmn*nlmn; ++ij) {
            hamiltonian_lmn[ij] = 0; // clear
            overlap_lmn[ij] = 0; // clear
        } // ij
        for(auto gnt : gaunt) {
            int const lm = gnt.lm, lm1 = gnt.lm1, lm2 = gnt.lm2; auto const G = gnt.G;
            if (lm1 < mlm && lm2 < mlm) {
                if (lm < nlm) {
                    for(int ilmn = lmn_begin[lm1]; ilmn < lmn_end[lm1]; ++ilmn) {
                        int const iln = ln_index_list[ilmn];
                        for(int jlmn = lmn_begin[lm2]; jlmn < lmn_end[lm2]; ++jlmn) {
                            int const jln = ln_index_list[jlmn];
                            hamiltonian_lmn[ilmn*nlmn + jlmn] +=
                              G * ( potential_ln[(lm*nln + iln)*nln + jln][TRU] 
                                  - potential_ln[(lm*nln + iln)*nln + jln][SMT] );
                        } // jlmn
                    } // ilmn
                } // lm
                if (0 == lm) {
    //              printf("# nln = %d\n", nln);
                    for(int ilmn = lmn_begin[lm1]; ilmn < lmn_end[lm1]; ++ilmn) {
                        int const iln = ln_index_list[ilmn];
                        for(int jlmn = lmn_begin[lm2]; jlmn < lmn_end[lm2]; ++jlmn) {
                            int const jln = ln_index_list[jlmn];
                            overlap_lmn[ilmn*nlmn + jlmn] += 
                              G * ( overlap_ln[iln*nln + jln][TRU]
                                  - overlap_ln[iln*nln + jln][SMT] );
                        } // jlmn
                    } // ilmn
                } // 0 == lm
            } // limits
        } // gnt

        // add the kinetic_energy deficit to the hamiltonian
        for(int ilmn = 0; ilmn < nlmn; ++ilmn) {
            int const iln = ln_index_list[ilmn];
            for(int jlmn = 0; jlmn < nlmn; ++jlmn) {
                int const jln = ln_index_list[jlmn];
                hamiltonian_lmn[ilmn*nlmn + jlmn] += kinetic_energy[iln*nln + jln][TRU]
                                                   - kinetic_energy[iln*nln + jln][SMT];
            } // jlmn
        } // ilmn
        // ToDo: find out if we miss some terms
        
        for(int ij = 0; ij < nSHO*matrix_stride; ++ij) {
            hamiltonian[ij] = 0; // clear
            overlap[ij] = 0; // clear
        } // ij
        // ToDo: transform _lmn quantities using sho_unitary
        
    } // update
    
    void update(int echo=0) {
        update_core_states(.5, echo);
        update_valence_states(echo);
        update_charge_deficit(echo);
        double density_matrix[1<<19], rho_tensor[1<<14], qlm[1<<17];
        get_rho_tensor(rho_tensor, density_matrix);
        update_full_density(rho_tensor);
        update_full_potential(qlm);
        update_matrix_elements();
    } // update

  }; // class LiveAtom


namespace single_atom {
  // this module allows to compute a full PAW calculation on a radial grid, 
  // i.e. we assume radial symmetry of density and potential and thus 
  // we can solve all equations on 1D radial grids.
  
  // Potential radial basis functions:
  //  * grid points
  //  * Bessel functions or 
  //  * radial SHO eigenfunctions

  // What is needed?
  //  * PAW construction of smooth partial waves (SHO-scheme), see ReSpawN or juRS
  //  * projection coefficients
  //  * Diagonalizer: LAPACK dsygv.f
  //  * self-consistency, e.g. by potential mixing
  // We have only up to 4 wave functions: s, p, d and f
  // We have only a monopole charge compensator (Gaussian)
  // Maybe we should write a live_atom module first 
  //   (a PAW generator prepared for core level und partial wave update)
  

#ifdef  NO_UNIT_TESTS
  status_t all_tests() { printf("\nError: %s was compiled with -D NO_UNIT_TESTS\n\n", __FILE__); return -1; }
#else // NO_UNIT_TESTS

  int test(int echo=9) {
    if (echo > 0) printf("\n%s: new struct live_atom has size %ld Byte\n\n", __FILE__, sizeof(LiveAtom));
    for(int Z = 29; Z <= 29; ++Z) {
        if (echo > 1) printf("\n# Z = %d\n", Z);      
        LiveAtom a(Z);
    }
    return 0;
  } // test

  status_t all_tests() {
    auto status = 0;
    status += test();
    return status;
  } // all_tests
#endif // NO_UNIT_TESTS  

} // namespace single_atom
