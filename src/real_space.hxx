#pragma once

#include <cstdint> // uint32_t
#include <algorithm> // std::max
#include <cstdio> // printf
#include <cassert> // assert

#include "inline_math.hxx" // set, scale
#include "bessel_transform.hxx" // Bessel_j0
#include "recorded_warnings.hxx" // warn
#include "boundary_condition.hxx" // Periodic_Boundary, Isolated_Boundary, Mirrored_Boundary, Invalid_Boundary

#include "status.hxx" // status_t

namespace real_space {
  
  int constexpr debug = 0;

  class grid_t {
  private:
      uint32_t dims[4]; // 0,1,2:real-space grid dimensions, 3:outer dim
      int bc[3]; // boundary conditions
  public:
      double h[3], inv_h[3]; // grid spacings and their inverse

      grid_t(void) : dims{0,0,0,1}, bc{0,0,0}, h{1,1,1}, inv_h{1,1,1} {}

      grid_t(int const d0, int const d1, int const d2, int const dim_outer=1)
       : bc{0,0,0}, h{1,1,1}, inv_h{1,1,1} {
          dims[0] = std::max(1, d0); // x
          dims[1] = std::max(1, d1); // y
          dims[2] = std::max(1, d2); // z
          dims[3] = std::max(1, dim_outer);
          long const nnumbers = dims[3] * dims[2] * dims[1] * dims[0];
          if (nnumbers > 0) {
              if (debug) printf("# grid with %d x %d x %d * %d = %.6f M numbers\n", 
                  dims[0], dims[1], dims[2], dims[3], nnumbers*1e-6);
          } else {
              if (debug) printf("# grid invalid: dims={%d, %d, %d,  %d}\n",
                      dims[0], dims[1], dims[2], dims[3]);
          }
      } // constructor

      template<typename int_t>
      grid_t(int_t const dim[3], int const dim_outer=1) : grid_t(dim[0], dim[1], dim[2], dim_outer) {} // delegating contructor

      ~grid_t() {
          long const nnumbers = dims[3] * dims[2] * dims[1] * dims[0];
          if (debug) printf("# release a grid with %d x %d x %d * %d = %.6f M numbers\n",
                                      dims[0], dims[1], dims[2], dims[3], nnumbers*1e-6);
      } // destructor

      status_t set_grid_spacing(double const hx, double const hy=-1, double const hz=-1) {
          status_t stat = 0;
          double const h3[3] = {hx, (hy<0)?hx:hy, (hz<0)?hx:hz};
          for(int i3 = 0; i3 < 3; ++i3) {
              h[i3] = h3[i3]; // convert to double
              if ((0.0 != h[i3]) && (h[i3] == h[i3])) {
                  inv_h[i3] = 1./h[i3]; // invert only here
              } else ++stat;
          } // i3
          return stat;
      } // set

      status_t set_boundary_conditions(int const bc3[3]) { set(bc, 3, bc3); return 0; }
      status_t set_boundary_conditions(int const bcx
                     , int const bcy=Invalid_Boundary 
                     , int const bcz=Invalid_Boundary) {
          bc[0] = bcx;
          bc[1] = (bcy == Invalid_Boundary) ? bcx : bcy;
          bc[2] = (bcz == Invalid_Boundary) ? bcx : bcz;
          return  (bcx == Invalid_Boundary);
      } // set

      inline int operator[] (int const d) const { assert(0 <= d); assert(d < 4); return dims[d]; }
      inline int operator() (char const c) const { assert('x' <= (c|32)); assert((c|32) <= 'z'); return dims[(c|32) - 120]; }
      inline double dV(bool const Cartesian=true) const { return h[0]*h[1]*h[2]; } // volume element, assuming a Cartesian grid
      inline double grid_spacing(int const d) const { assert(0 >= d); assert(d < 3); return h[d]; } // so far not used
      inline double const * grid_spacings() const { return h; } // so far not used
      inline double smallest_grid_spacing() const { return std::min(std::min(h[0], h[1]), h[2]); }
      inline size_t all() const { return ((size_t(dims[3]) * dims[2]) * dims[1]) * dims[0]; }
      inline int boundary_condition(int  const d) const { assert(0 <= d); assert(d < 3); return bc[d]; }
      inline int boundary_condition(char const c) const { return boundary_condition((c|32) - 120); }
      inline int const * boundary_conditions() const { return bc; }
      inline bool all_boundary_conditions_periodic() const { return (Periodic_Boundary == bc[0]) && 
                                    (Periodic_Boundary == bc[1]) && (Periodic_Boundary == bc[2]); };
  }; // class grid_t
  
  
  
  template<typename real_t> // inner dimension
  status_t add_function(real_t values[], grid_t const &g, real_t *added, //
                        double const r2coeff[], int const ncoeff, float const hcoeff,
                        double const center[3]=nullptr, double const factor=1, float const rcut=-1) {
  // Add a spherically symmetric regular function to the grid.
  // The function is tabulated as r2coeff[0 <= hcoeff*r^2 < ncoeff][D0]
      status_t stat(0);
      double c[3] = {0,0,0}; if (center) set(c, 3, center);
      bool const cutoff = (rcut >= 0); // negative values mean that we do not need a cutoff
      double const r2cut = cutoff ? rcut*rcut : (ncoeff - 1)/hcoeff;
      int imn[3], imx[3];
      size_t nwindow = 1;
      for(int d = 0; d < 3; ++d) {
          int const M = g[d] - 1; // highest index
          if (cutoff) {
              imn[d] = std::max(0, (int)std::floor((c[d] - rcut)*g.inv_h[d]));
              imx[d] = std::min(M, (int)std::ceil ((c[d] + rcut)*g.inv_h[d]));
          } else {
              imn[d] = 0;
              imx[d] = M;
          } // cutoff
#ifdef  DEBUG
          printf("# %s window %c = %d elements from %d to %d\n", __func__, 'x'+d, imx[d] + 1 - imn[d], imn[d], imx[d]);
#endif
          nwindow *= std::max(0, imx[d] + 1 - imn[d]);
      } // d
      assert(hcoeff > 0);
      real_t added_charge{0}; // clear
      size_t modified = 0, out_of_range = 0;
      for(            int iz = imn[2]; iz <= imx[2]; ++iz) {  double const vz = iz*g.h[2] - c[2], vz2 = vz*vz;
          for(        int iy = imn[1]; iy <= imx[1]; ++iy) {  double const vy = iy*g.h[1] - c[1], vy2 = vy*vy;
              if (vz2 + vy2 < r2cut) {
                  for(int ix = imn[0]; ix <= imx[0]; ++ix) {  double const vx = ix*g.h[0] - c[0], vx2 = vx*vx;
                      double const r2 = vz2 + vy2 + vx2;
                      if (r2 < r2cut) {
                          int const ixyz = (iz*g('y') + iy)*g('x') + ix;
                          int const ir2 = (int)(hcoeff*r2);
                          if (ir2 < ncoeff) {
                              double const w8 = hcoeff*r2 - ir2; // linear interpolation weight
                              int const ir2p1 = ir2 + 1;
                                  auto const value_to_add = (r2coeff[ir2]*(1 - w8)
                                      + ((ir2p1 < ncoeff) ? r2coeff[ir2p1] : 0)*w8);
                                  values[ixyz] += factor*value_to_add;
                                  added_charge  += factor*value_to_add;
#if 0
//       printf("#rs %g %g\n", std::sqrt(r2), value_to_add);
//        printf("#rs %.1f %.1f %.1f %.12f\n", vx*g.inv_h[0], vy*g.inv_h[1], vz*g.inv_h[2], value_to_add);
#endif
                              ++modified;
                          } else ++out_of_range;
                      } // inside rcut
                  } // ix
              } // rcut for (y,z)
          } // iy
      } // iz
      *added = added_charge * g.dV(); // volume integral
#ifdef  DEBUG
      printf("# %s modified %.3f k inside a window of %.3f k on a grid of %.3f k grid values.\n", 
              __func__, modified*1e-3, nwindow*1e-3, g('x')*g('y')*g('z')*1e-3); // show stats
#endif
      if (out_of_range > 0) {
          stat += 0 < warn("Found %ld entries out of range of the radial function!\n", out_of_range);
      } // out of range of the radial function
      return stat;
  } // add_function

  template<typename real_t>
  status_t bessel_projection(real_t q_coeff[], int const nq, float const dq,
                real_t const values[], grid_t const &g, double const center[3]=nullptr, 
                float const rcut=-1, double const factor=1) {
      double c[3] = {0,0,0}; if (center) set(c, 3, center);
      bool const cutoff = (rcut >= 0); // negative values mean that we do not need a cutoff
      double const r2cut = cutoff ? rcut*rcut : 100.; // stop at 10 Bohr
      int imn[3], imx[3];
      size_t nwindow = 1;
      for(int d = 0; d < 3; ++d) {
          int const M = g[d] - 1; // highest index
          if (cutoff) {
              imn[d] = std::max(0, (int)std::floor((c[d] - rcut)*g.inv_h[d]));
              imx[d] = std::min(M, (int)std::ceil ((c[d] + rcut)*g.inv_h[d]));
          } else {
              imn[d] = 0;
              imx[d] = M;
          } // cutoff
#ifdef  DEBUG
          printf("# %s window %c = %d elements from %d to %d\n", __func__, 'x'+d, imx[d] + 1 - imn[d], imn[d], imx[d]);
#endif
          nwindow *= std::max(0, imx[d] + 1 - imn[d]);
      } // d
      set(q_coeff, nq, real_t(0)); // clear
      for(            int iz = imn[2]; iz <= imx[2]; ++iz) {  double const vz = iz*g.h[2] - c[2], vz2 = vz*vz;
          for(        int iy = imn[1]; iy <= imx[1]; ++iy) {  double const vy = iy*g.h[1] - c[1], vy2 = vy*vy;
              if (vz2 + vy2 < r2cut) {
                  for(int ix = imn[0]; ix <= imx[0]; ++ix) {  double const vx = ix*g.h[0] - c[0], vx2 = vx*vx;
                      double const r2 = vz2 + vy2 + vx2;
                      if (r2 < r2cut) {
                          int const ixyz = (iz*g('y') + iy)*g('x') + ix;
                          double const r = std::sqrt(r2);
//                           printf("%g %g\n", r, values[ixyz]); // DEBUG
                          for(int iq = 0; iq < nq; ++iq) {
                              double const q = iq*dq;
                              double const x = q*r;
                              double const j0 = bessel_transform::Bessel_j0(x);
                              q_coeff[iq] += values[ixyz] * j0;
                          } // iq
                      } // inside rcut
                  } // ix
//                   printf("\n"); // DEBUG
              } // rcut for (y,z)
          } // iy
      } // iz
      double const sqrt2pi = std::sqrt(2./constants::pi); // this makes the transform symmetric
      scale(q_coeff, nq, real_t(g.dV()*factor*sqrt2pi)); // volume element, external factor, Bessel transform factor
      return 0; // success
  } // bessel_projection

  status_t all_tests(int const echo=0);

} // namespace real_space