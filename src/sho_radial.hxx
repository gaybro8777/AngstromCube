#pragma once

#include "constants.hxx" // sqrtpi

typedef int status_t;

namespace sho_radial {
  
  inline int constexpr nSHO(int const numax) { return ((3 + numax)*(2 + numax)*(1 + numax))/6; } // number of SHO eigenstates
  inline int constexpr nSHO_radial(int const numax) { return (numax*(numax + 4) + 4)/4; } // number of different radial eigenstates

  template<typename real_t>
  void radial_eigenstates(real_t poly[], // coefficients of a polynomial in r^2
                   int const nrn, // number of radial nodes
                   int const ell, // angular momentum quantum number
                   real_t const factor=1) { // if we know the normalization prefactor in advance, we can provide it here

    // recursion relation of coefficients:
    // a_0 = 1, a_{k+1} = (k-nrn)/((k+1)*(ell+k+3/2)) a_k

    poly[0] = factor;
    for(int k = 0; k < nrn; ++k) {
        // from https://quantummechanics.ucsd.edu/ph130a/130_notes/node244.html
        poly[k + 1] = (poly[k]*(k - nrn)*2)/(real_t)((k + 1)*(2*ell + 2*k + 3));
    } // k

  } // radial_eigenstates

  template<typename real_t>
  real_t exponential_integral_k(int const k) {
    // I_k = int\limit_0^\infty dr r^k exp(-r^2)
    if (0 == k) return 0.5*constants::sqrtpi;
    if (1 == k) return 0.5; // just for completeness, odd cases are not relevant in this module
    assert(k > 0);
    return (real_t)(0.5*(k - 1))*exponential_integral_k<real_t>(k - 2); // recursive invokation
  } // exponential_integral_k

  template<typename real_t>
  real_t radial_normalization(real_t const coeff[], int const nrn, int const ell) {

    auto const prod = new real_t[2*nrn + 1]; // coefficients of a polynomial in r^2
    for(int p = 0; p < 2*nrn + 1; ++p) {
        prod[p] = 0;
    } // p
    for(int k = 0; k <= nrn; ++k) {
        for(int p = 0; p <= nrn; ++p) {
            prod[k + p] += coeff[k]*coeff[p]; // polynomial product with itself
        } // p
    } // k

    real_t exp_int_k = exponential_integral_k<real_t>(2*ell + 2);
    real_t norm = 0;
    for(int p = 0; p <= 2*nrn; ++p) { // loop must run serial forward
//    assert(exp_int_k == exponential_integral_k<real_t>(2*p + 2*ell + 2)); // DEBUG
      norm += prod[p]*exp_int_k;
      exp_int_k *= (p + ell + 1.5); // prepare exp_int_k for the next iteration
    } // p
    delete[] prod;
    return 1./std::sqrt(norm); // normalization prefactor
  } // radial_normalization
  
  template<typename real_t>
  real_t radial_normalization(int const nrn, int const ell) {
    auto const coeff = new real_t[nrn + 1]; // coefficients of a polynomial in r^2
    radial_eigenstates(coeff, nrn, ell); // get polynomial coefficients of the SHO eigenstates
    auto const result = radial_normalization(coeff, nrn, ell);
    delete[] coeff;
    return result;
  } // radial_normalization

  template<typename real_t>
  real_t expand_poly(real_t const coeff[], int const ncoeff, double const x) {
      real_t value = 0;
      double xpow = 1;
      for(int i = 0; i < ncoeff; ++i) {
          value += coeff[i] * xpow;
          xpow *= x;
      } // i
      return value;
  } // expand_poly
  
  status_t all_tests();
  
} // namespace sho_radial
