#pragma once

#include <cstdint> // uint32_t
#include <cstdio> // printf

typedef int status_t;

namespace finite_difference {

  template<typename real_t>
  status_t set_Laplacian_coefficients(real_t c[], int const nn=1, 
                                   double const grid_spacing=1) {
    double const h2 = grid_spacing*grid_spacing;
    int constexpr nnMaxImplemented = 13;
    switch (std::min(nn, nnMaxImplemented)) {
    case 1: // use 3 points, lowest order
      c[1] = 1./(1.*h2);
      c[0] = -2./(1.*h2);

    break; case 2: // use 5 points
      c[2] = -1./(12.*h2);
      c[1] = 4./(3.*h2);
      c[0] = -5./(2.*h2);

    break; case 3: // use 7 points
      c[3] = 1./(90.*h2);
      c[2] = -3./(20.*h2);
      c[1] = 3./(2.*h2);
      c[0] = -49./(18.*h2);

    break; case 4: // use 9 points
      c[4] = -1./(560.*h2);
      c[3] = 8./(315.*h2);
      c[2] = -1./(5.*h2);
      c[1] = 8./(5.*h2);
      c[0] = -205./(72.*h2);

    break; case 5: // use 11 points
      c[5] = 1./(3150.*h2);
      c[4] = -5./(1008.*h2);
      c[3] = 5./(126.*h2);
      c[2] = -5./(21.*h2);
      c[1] = 5./(3.*h2);
      c[0] = -5269./(1800.*h2);

    break; case 6: // use 13 points
      c[6] = -1./(16632.*h2);
      c[5] = 2./(1925.*h2);
      c[4] = -1./(112.*h2);
      c[3] = 10./(189.*h2);
      c[2] = -15./(56.*h2);
      c[1] = 12./(7.*h2);
      c[0] = -5369./(1800.*h2);

    break; case 7: // use 15 points
      c[7] = 1./(84084.*h2);
      c[6] = -7./(30888.*h2);
      c[5] = 7./(3300.*h2);
      c[4] = -7./(528.*h2);
      c[3] = 7./(108.*h2);
      c[2] = -7./(24.*h2);
      c[1] = 7./(4.*h2);
      c[0] = -266681./(88200.*h2);

    break; case 8: // use 17 points
      c[8] = -1./(411840.*h2);
      c[7] = 16./(315315.*h2);
      c[6] = -2./(3861.*h2);
      c[5] = 112./(32175.*h2);
      c[4] = -7./(396.*h2);
      c[3] = 112./(1485.*h2);
      c[2] = -14./(45.*h2);
      c[1] = 16./(9.*h2);
      c[0] = -1077749./(352800.*h2);

    break; case 9: // use 19 points
      c[9] = 1./(1969110.*h2);
      c[8] = -9./(777920.*h2);
      c[7] = 9./(70070.*h2);
      c[6] = -2./(2145.*h2);
      c[5] = 18./(3575.*h2);
      c[4] = -63./(2860.*h2);
      c[3] = 14./(165.*h2);
      c[2] = -18./(55.*h2);
      c[1] = 9./(5.*h2);
      c[0] = -9778141./(3175200.*h2);

    break; case 10: // use 21 points
      c[10] = -1./(9237800.*h2);
      c[9] = 10./(3741309.*h2);
      c[8] = -5./(155584.*h2);
      c[7] = 30./(119119.*h2);
      c[6] = -5./(3432.*h2);
      c[5] = 24./(3575.*h2);
      c[4] = -15./(572.*h2);
      c[3] = 40./(429.*h2);
      c[2] = -15./(44.*h2);
      c[1] = 20./(11.*h2);
      c[0] = -1968329./(635040.*h2);

    break; case 11: // use 23 points
      c[11] = 1./(42678636.*h2);
      c[10] = -11./(17635800.*h2);
      c[9] = 11./(1360476.*h2);
      c[8] = -55./(806208.*h2);
      c[7] = 55./(129948.*h2);
      c[6] = -11./(5304.*h2);
      c[5] = 11./(1300.*h2);
      c[4] = -11./(364.*h2);
      c[3] = 55./(546.*h2);
      c[2] = -55./(156.*h2);
      c[1] = 11./(6.*h2);
      c[0] = -239437889./(76839840.*h2);

    break; case 12: // use 25 points
      c[12] = -1./(194699232.*h2);
      c[11] = 12./(81800719.*h2);
      c[10] = -3./(1469650.*h2);
      c[9] = 44./(2380833.*h2);
      c[8] = -33./(268736.*h2);
      c[7] = 132./(205751.*h2);
      c[6] = -11./(3978.*h2);
      c[5] = 396./(38675.*h2);
      c[4] = -99./(2912.*h2);
      c[3] = 88./(819.*h2);
      c[2] = -33./(91.*h2);
      c[1] = 24./(13.*h2);
      c[0] = -240505109./(76839840.*h2);

    break; case 13: // use 27 points
      c[13] = 1./(878850700.*h2);
      c[12] = -13./(374421600.*h2);
      c[11] = 13./(25169452.*h2);
      c[10] = -13./(2600150.*h2);
      c[9] = 13./(366282.*h2);
      c[8] = -143./(723520.*h2);
      c[7] = 143./(158270.*h2);
      c[6] = -143./(40698.*h2);
      c[5] = 143./(11900.*h2);
      c[4] = -143./(3808.*h2);
      c[3] = 143./(1260.*h2);
      c[2] = -13./(35.*h2);
      c[1] = 13./(7.*h2);
      c[0] = -40799043101./(12985932960.*h2);
      
    break; case 0:
      printf("Warning! Finite difference switched off!\n");
      c[0] = 0.;
      return 0; // ok
    break; default:
      printf("Warning! Cannot treat case of %d finite difference neighbors\n", nn);
      return nn;
    } // switch min(nn, nnMaxImplemented)
    
    if (nn > nnMaxImplemented) {
      printf("Warning! Max implemented 13 but requested %d finite difference neighbors\n", nn);
      for(int i = nnMaxImplemented + 1; i <= nn; ++i) c[i] = 0; // clear out the others.
    } // larger than implemented
    
    return 0;
  } // set_Laplacian_coefficients

  
  template<typename real_t>
  class finite_difference_t {
    public:
      int bc[3][2]; // boundary conditions, lower and upper
      double h[3]; // grid spacings
      int nn[3]; // number of FD neighbors
      real_t c2nd[3][16]; // coefficients for the 2nd derivative
    public:
      finite_difference_t(double const grid_spacing=1, int const boundary_condition=1, int const nneighbors=4) {
          for(int d = 0; d < 3; ++d) {
              for(int i = 0; i < 16; ++i) c2nd[d][i] = 0; // clear
              nn[d] = nneighbors;
              h[d] = grid_spacing;
              bc[d][0] = bc[d][1] = d - 1;
              auto const stat = set_Laplacian_coefficients(c2nd[d], nn[d], h[d]);
              if (stat) printf("# Warning! construction of finite_difference_t failed for direction=%c\n", 'x'+d);
          } // spatial direction d
      } // constructor
  }; // class finite_difference_t
  
  
  status_t all_tests();

} // namespace finite_difference
