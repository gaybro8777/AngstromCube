#pragma once

#include <cmath> // std::exp

#include "radial_grid.h" // radial_grid_t
#include "quantum_numbers.h" // ell_QN_t

typedef int status_t;

namespace atom_core {

  status_t scf_atom(
      radial_grid_t const &g, // radial grid descriptor
      float const Z, // atomic number
      int const echo); // log output level

  status_t read_Zeff_from_file(double Zeff[], radial_grid_t const &g, float const Z,
                   char const name[]="Zeff", float const factor=1, int const echo=9);

  double initial_density(double r2rho[], radial_grid_t const &g, double const Z, double const charged=0);

  void rad_pot(double rV[], radial_grid_t const &g, double const rho4pi[], double const Z=0, double *energies=nullptr);

  inline double guess_energy(double const Z, int const enn) {
      return -.5*(Z/enn)*(Z/enn) *  // Hydrogen-like energies in the Hartree unit system
            (.783517 + 2.5791E-5*(Z/enn)*(Z/enn)) * // fit for the correct 1s energy
            std::exp(-.01*(enn - 1)*Z); // guess energy
  } // guess_energy

  inline int nl_index(int const enn, int const ell) { 
      assert(ell >= 0); assert(enn > ell); // atomic eigenstates
      return (enn*(enn - 1))/2 + ell;
  } // nl_index

  inline char ellchar(ell_QN_t const ell) {
      char constexpr special_ellchars[] = "spd";
      if (ell < 0) return '?';
      if (ell < 3) return special_ellchars[ell];
      return 99 + ell; // "fghijk ..."
  } // ellchar
  
  status_t all_tests();

} // namespace atom_core
