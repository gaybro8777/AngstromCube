#include <cstdio> // printf
#include <cmath> // cos, abs, sqrt
#include <cassert> // assert
#include <algorithm> // fill

#include "fourier_poisson.hxx"

#ifndef HAS_no_MKL
  #include "mkl_dfti.h" // Intel(c) Math Kernel Library, discrete Fourier transform interface
#endif

#ifdef HAS_FFTW
extern "C" {
  #include "fftw.h" // fftw_plan, fftw_plan_dft_r2r_3d(int n0, int n1, int n2, double *in, double *out, 
                    //    fftw_r2r_kind kind0, fftw_r2r_kind kind1, fftw_r2r_kind kind2, unsigned flags)
                    //    FFTW_REDFT00
}
#endif

#include "constants.hxx" // pi
#include "inline_tools.hxx" // align
#include "vector_math.hxx" // vec<n,T>
#include "inline_math.hxx" // pow2, pow3

#define call(x) { auto const error_status = (x); if (error_status) return error_status; }

namespace fourier_poisson {

  template<typename real_t>
  status_t fft_MKL(real_t out[] // (out) indexing out[(iz*ng[1] + iy)*ng[0] + ix]
                 , real_t imag[] // imaginary part of in when backward, imaginary part of out when forward
                 , real_t const in[] // (in) indexing in[(iz*ng[1] + iy)*ng[0] + ix]
                 , int const ng[3] // grid numbers
                 , char const direction='f' // in the case of backward, the representations are swapped
                  ) {
#ifndef HAS_no_MKL   
      MKL_LONG status;
      MKL_LONG const l[3] = {ng[2], ng[1], ng[0]};
      size_t const ngall = l[2]*l[1]*l[0];
      DFTI_DESCRIPTOR_HANDLE my_desc_handle;
      status = DftiCreateDescriptor(&my_desc_handle, (sizeof(real_t) > 4) ? DFTI_DOUBLE : DFTI_SINGLE, DFTI_COMPLEX, 3, l);
      status = DftiSetValue(my_desc_handle, DFTI_COMPLEX_STORAGE, DFTI_REAL_REAL);
      status = DftiSetValue(my_desc_handle, DFTI_PLACEMENT, DFTI_NOT_INPLACE);
      status = DftiCommitDescriptor(my_desc_handle);
      auto imag2 = new real_t[ngall];
      if ('f' == direction) { // forward
          std::fill(imag2, imag2 + ngall, 0); // assume that array in is real, so its imaginary part (here imag2) is zero
          status = DftiComputeForward (my_desc_handle, (void*)in, (void*)imag2, (void*)out, (void*)imag); // perform the forward FFT
      } else {
          // in the backtransform we are not interested in the imaginary part of the output, so the content of imag2 is ignored
          status = DftiComputeBackward(my_desc_handle, (void*)in, (void*)imag, (void*)out, (void*)imag2); // perform the forward FFT
      }
      status = DftiFreeDescriptor(&my_desc_handle); // cleanup, can be moved out
      delete[] imag2;
      return status;
#else
      return -1; // error
#endif      
  } // fft_MKL
  
  
  template<typename real_t>
  status_t fourier_solve(real_t x[] // (out) solution of Laplacian*x == b
                       , real_t const b[] // right hand side b
                       , int const ng[3] // grid numbers
                       , double const reci[3][4] // shape of the reciprocal space
                       , double const factor) {
      
      size_t const ng_all = 1ul * ng[0] * ng[1] * ng[2];
      auto const mg_all = align<3>(ng_all); // aligned to 8 real_t numbers
      auto const x_Re = new real_t[2*mg_all]; // get memory
      auto const x_Im = x_Re + mg_all; // point to the second half of that array

      status_t stat = 0;
      stat += fft_MKL(x_Re, x_Im, b, ng); // transform b into reciprocal space

      printf("# %s charge neutrality = %g %g\n", __func__, x_Re[0], x_Im[0]);
      x_Re[0] = 0; x_Im[0] = 0; // charge neutrality, clear the k=[0 0 0]-component

      real_t const scale = -factor/ng_all;

      typedef vector_math::vec<3,double> vec3;
      vec3 rec[3]; for(int d = 0; d < 3; ++d) rec[d] = reci[d];

      int const nh[3] = {ng[0]/2, ng[1]/2, ng[2]/2};

      for(int j2 = 0; j2 < ng[2]; ++j2) {         int const k2 = j2 - (j2 > nh[2])*ng[2]; vec3 const vec2   = rec[2]*k2;
          for(int j1 = 0; j1 < ng[1]; ++j1) {     int const k1 = j1 - (j1 > nh[1])*ng[1]; vec3 const vec21  = rec[1]*k1 + vec2;
              for(int j0 = 0; j0 < ng[0]; ++j0) { int const k0 = j0 - (j0 > nh[0])*ng[0]; vec3 const vec210 = rec[0]*k0 + vec21;
                  int const i = (j2*ng[1] + j1)*ng[0] + j0;

                  int const kk = k0*k0 + k1*k1 + k2*k2;
                  if (kk > 0) {
                      real_t const invLaplacian = scale/norm(vec210);
                      // modify x_Re and x_Im in-place
                      x_Re[i] *= invLaplacian;
                      x_Im[i] *= invLaplacian;
                  } // kk > 0

              } // j0
          } // j1
      } // j2
      
      stat += fft_MKL(x, x_Im, x_Re, ng, 'b'); // transform solution x back into real-space

      delete[] x_Re; // deallocates x_Re and also x_Im
      return stat;
  } // fourier_solve

  
  status_t fourier_solve_MKL(double x[] // (out) solution of Laplace*x == b
                        , double const b[] // right hand side b
                        , int const ng[3] // grid numbers
                        , double const bravais[3][4] // unit cell extend
                        ) {


      return 0;
  } // fourier_solve_MKL

#ifdef  NO_UNIT_TESTS
  status_t all_tests() { printf("\nError: %s was compiled with -D NO_UNIT_TESTS\n\n", __FILE__); return -1; }
#else // NO_UNIT_TESTS

  template<typename real_t>
  int test_fft(int const echo=6) {
      printf("\n# %s:\n", __func__);
      int const ng[3] = {29, 13, 9};
      int const ngall = ng[2]*ng[1]*ng[0];
      auto rs = new real_t[ngall];
      double const pw[3] = {3./ng[0], 2./ng[1], 1./ng[2]};
      if (echo > 1) printf("# %s: set up a single plane wave as [%g %g %g]\n", __func__, pw[0]*ng[0], pw[1]*ng[1], pw[2]*ng[2]);
      for(int z = 0; z < ng[2]; ++z) {
      for(int y = 0; y < ng[1]; ++y) {
      for(int x = 0; x < ng[0]; ++x) {
                  int const i = (z*ng[1] + y)*ng[0] + x;
                  rs[i] = std::cos(2*constants::pi*((pw[0]*x + pw[1]*y + pw[2]*z)));
      }}} // zyx
      auto ft = new real_t[2*ngall]; auto ft_imag = ft + ngall; // two arrays ft[_real] and ft_imag are adjacent in memory
      auto const status_fft = fft_MKL(ft, ft_imag, rs, ng, 'f'); // forward
      real_t maximum = 0; int at[4] = {-1,-1,-1,-1};
      for(int reim = 0; reim < 2; ++reim) {
      for(int z = 0; z < ng[2]; ++z) {
      for(int y = 0; y < ng[1]; ++y) {
      for(int x = 0; x < ng[0]; ++x) {
                  int const i = (z*ng[1] + y)*ng[0] + x;
                  auto const fta = std::abs(ft[reim*ngall + i]);
                  if (fta > maximum) { maximum = fta; at[0] = x; at[1] = y; at[2] = z; at[3] = reim; }
      }}}} // czyx
      if (echo > 5) printf("# %s: detected peak at index [%d %d %d] %s-part, value %g\n", 
                      __func__, at[0], at[1], at[2], (at[3])?"imag":"real", maximum);
      auto rs_back = new real_t[ngall];
      auto const status_inv = fft_MKL(rs_back, ft_imag, ft, ng, 'b'); // backward
      if (echo > 8) printf("\n# %s: back-transformed cos-wave values:\n", __func__);
      real_t const omega_inv = 1./ngall;
      double deva = 0, dev2 = 0;
      for(int z = 0; z < ng[2]; ++z) {
      for(int y = 0; y < ng[1]; ++y) {
      for(int x = 0; x < ng[0]; ++x) {
                  int const i = (z*ng[1] + y)*ng[0] + x;
                  auto const d = rs_back[i]*omega_inv - rs[i];
                  deva += std::abs(d); dev2 += d*d;
                  if (echo > 8) printf("%d %g %g %g\n", i, rs_back[i]*omega_inv, rs[i], d);
      }}} // zyx
      if (echo > 2) printf("# back-transformed cos-wave differs abs %.1e rms %.1e\n", deva/ngall, std::sqrt(dev2/ngall));
      delete[] rs_back;
      delete[] ft;
      delete[] rs;
      return status_fft + status_inv;
  } // test_fft

  int test_FFT_Poisson_solver(int const echo=3) {
      if (echo > 1) printf("\n# %s:\n", __func__);
      auto const pi = constants::pi;
      status_t stat = 0;
      int const ng[3] = {32, 32, 32}, ngall = ng[2]*ng[1]*ng[0];
      double const mat[3][4] = {{2*pi/ng[0],0,0, 0},{0,2*pi/ng[1],0, 0}, {0,0,2*pi/ng[2], 0}};
      double const alpha = 1./pow2(8.); // in units of h^-2
      auto rho = new double[ngall];
      auto V   = new double[ngall];
      double charge = 0, q_rad = 0, V_rad = 0; // charge
      for(int i01 = 0; i01 <= 1; ++i01) {
          double q = 0; // count charge
          for(int z = 0; z < ng[2]; ++z) {
          for(int y = 0; y < ng[1]; ++y) {
          for(int x = 0; x < ng[0]; ++x) {
                      double const r2 = pow2(x - .5*ng[0]) + pow2(y - .5*ng[1]) + pow2(z - .5*ng[0]);
                      int const i = (z*ng[1] + y)*ng[0] + x;
                      rho[i] = std::exp(-alpha*r2) - charge;
                      q += rho[i];
                      if (i01 && (echo > 6)) printf("%g %g %g\n", std::sqrt(r2), rho[i], V[i]);
          }}} // zyx
          if (0 == i01) {
              stat += fourier_solve(V, rho, ng, mat);
              charge = q/ngall;
              if (echo > 2) printf("# charge in cell %g %g\n", q, charge);
          } else {
              if (echo > 2) printf("# charge in cell %g %g\n", q, charge);
          }
      } // i01
      if (echo > 4) printf("\n# radial density and 1/r Coulomb potential\n");
      double const dr = 1./8.;
      for(int i01 = 0; i01 <= 1; ++i01) {
          q_rad = 0;
          for(int ir = 0; ir < ng[0]/dr; ++ir) {
              auto const r = (ir + .125)*dr, r2 = r*r;
              auto const rho_rad = std::exp(-alpha*r2) - charge;
              // show density, (shifted) potential, integrated charge up to r
              if (i01 && (echo > 4)) printf("%g %g %g %g\n", r, rho_rad, V_rad + q_rad/r, q_rad); 
              q_rad += rho_rad * 4*pi * r2 * dr;
              V_rad -= rho_rad * 4*pi * r * dr * (2*i01 - 1); // sum up in 1st iteration and subtract in second
          } // ir
          if (echo > 3) printf("\n# radial integrated charge %g, V_rad %g\n", q_rad, V_rad);
      } // i01
      delete[] V;
      delete[] rho;
      return stat;
  } // test_FFT_Poisson_solver

  status_t all_tests() {
    auto status = 0;
    status += test_fft<float>();
    status += test_fft<double>();
    status += test_FFT_Poisson_solver();
    return status;
  } // all_tests
#endif // NO_UNIT_TESTS
  
} // namespace fourier_poisson
