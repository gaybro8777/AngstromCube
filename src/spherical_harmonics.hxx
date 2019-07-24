#pragma once

#include <complex> // std::complex<>
#include <cmath> // std::sqrt
#include "constants.hxx" // constants::pi

namespace spherical_harmonics {

  template<typename real_t, typename vector_real_t>
  void Ylm(std::complex<real_t> ylm[], int const ellmax, vector_real_t const v[3]) {
    if (ellmax < 0) return;
// !************************************************************
// !     generate the spherical harmonics for the vector v
// !     using a stable upward recursion in l.  (see notes
// !     by m. weinert.)
// !          m.weinert   january 1982
// !     modified by R. Podloucky (added in ynorm); July 1989
// !     cleaned up    mw 1995
// !
// !     modified to make use of f90 constructs. note that
// !     the normalization is an internal subroutine and hence
// !     can only be called from here. also, no need to dimension
// !     arrays for ynorm, done dynamically.          mw 1999
// !************************************************************

      real_t constexpr small = 1e-12;


// !---> check whether  or not normalizations are needed
      static real_t *ynorm = nullptr;
      static int ellmaxd = -1; // -1:not_initalized

      if (ellmax > ellmaxd) {
// !-->     first deallocate the array if it exists
          if (nullptr != ynorm) {
              delete[] ynorm;
              printf("# %s resize table of normalization constants from %d to %d\n", __func__, (1 + ellmaxd)*(1 + ellmaxd), (1 + ellmax)*(1 + ellmax));
          } // resize
          ynorm = new real_t[(1 + ellmax)*(1 + ellmax)];

// !********************************************************************
// !     normalization constants for ylm (internal subroutine has access
// !     to ellmax and ynorm from above)
// !********************************************************************
          {   double const fpi = 4*constants::pi; // 4*pi
              for(int l = 0; l <= ellmax; ++l) {
                  int const lm0 = l*l + l;
                  double const a = std::sqrt((2*l + 1.)/fpi);
                  double cd = 1;
                  ynorm[lm0] = a;
                  double sgn = -1;
                  for(int m = 1; m <= l; ++m) {
                      cd /= ((l + 1. - m)*(l + m));
                      auto const yn = a*std::sqrt(cd);
                      ynorm[lm0 + m] = yn;
                      ynorm[lm0 - m] = yn * sgn;
                      sgn = -sgn; // prepare (-1)^m for the next iteration
                  } // m
              } // l
          } // scope to full ynorm with values
          ellmaxd = ellmax; // set static variable
      } else if (ellmax < 0 && nullptr != ynorm) {
          delete[] ynorm; // cleanup
      }

// !--->    calculate sin and cos of theta and phi
      auto const x = v[0], y = v[1], z = v[2];
      auto const xy2 = x*x + y*y;
      auto const r = std::sqrt(xy2 + z*z);
      auto const rxy = std::sqrt(xy2);

      real_t cth, sth, cph, sph;
      if (r > small) {
         cth = z/r;
         sth = rxy/r;
      } else {
         sth = 0;
         cth = 1;
      }
      if (rxy > small) {
         cph = x/rxy;
         sph = y/rxy;
      } else {
         cph = 1;
         sph = 0;
      }

      int const S = (1 + ellmax); // stride for p, the array of associated legendre functions
      real_t p[(1 + ellmax)*S];

// !---> generate associated legendre functions for m >= 0
      real_t fac = 1;
// !---> loop over m values
      for(int m = 0; m < ellmax; ++m) {
          fac *= (1 - 2*m);
          p[m     + S*m] = fac;
          p[m + 1 + S*m] = (m + 1 + m)*cth*fac;
// !--->    recurse upward in l
          for(int l = m + 2; l <= ellmax; ++l) {
              p[l + S*m] = ((2*l - 1)*cth*p[l - 1 + S*m] - (l + m - 1)*p[l - 2 + S*m])/((real_t)(l - m));
          } // l
          fac *= sth;
      } // m
      p[ellmax + S*ellmax] = (1 - 2*ellmax)*fac;

      real_t c[1 + ellmax], s[1 + ellmax];
// !--->    determine sin and cos of phi
      c[0] = 1; s[0] = 0;
      if (ellmax > 0) {
          c[1] = cph; s[1] = sph; 
          auto const cph2 = 2*cph;
          for(int m = 2; m <= ellmax; ++m) {
              s[m] = cph2*s[m - 1] - s[m - 2];
              c[m] = cph2*c[m - 1] - c[m - 2];
          } // m
      } // ellmax > 0

// !--->    multiply in the normalization factors
      for(int m = 0; m <= ellmax; ++m) {
          for(int l = m; l <= ellmax; ++l) {
              int const lm0 = l*l + l;
              auto const ylms = p[l + S*m]*std::complex<real_t>(c[m], s[m]);
              ylm[lm0 + m] = ynorm[lm0 + m]*ylms;
              ylm[lm0 - m] = ynorm[lm0 - m]*std::conj(ylms);
          } // l
      } // m

      return;
  } // Ylm

//   int all_tests();
  
} // namespace spherical_harmonics
