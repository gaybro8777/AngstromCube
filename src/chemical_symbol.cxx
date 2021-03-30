#include <cstdio> // printf, std::snprintf
#include <cassert> // assert
#include <cmath> // std::abs
#include <vector> // std::vector<T>

#include "chemical_symbol.hxx"

#include "chemical_symbol.h" // char element_symbols[128*2]

namespace chemical_symbol {

  int8_t get(char Sy[4], double const Z, char const blank) {
      if (nullptr == Sy) return -1; // error code -1: result pointer is null
      int const iZ = std::round(Z);
      auto const z7 = static_cast<int8_t>(iZ & 127); // Z % (2^7)
      char const S = element_symbols[z7*2 + 0];
      char const y = element_symbols[z7*2 + 1];
      Sy[0] = S;
      Sy[1] = (' ' == y) ? blank : y;
      Sy[2] = blank;
      Sy[3] = 0; // null-termination
      if (iZ != Z) { Sy[1 + (' ' != y)] = '*'; } // add an asterik
      return z7;
  } // get

  bool constexpr allow_digit_reading = true;
  bool constexpr allow_leading_zero  = true;
  
  // strategy: combine both printable ASCII char S,y into one 16 bit number
  inline uint16_t constexpr encode_16bit(char const S, char const y) { return S | ((y?y:' ') << 8); }

  int8_t decode(char const S, char const y) {
      switch  (encode_16bit(S, y)) {
          // from here on, the code is auto-generated by test_consistency, see below
          case encode_16bit('_','_'): return   0; // "__"
          case encode_16bit('H',' '): return   1; // "H "
          case encode_16bit('H','e'): return   2; // "He"
          case encode_16bit('L','i'): return   3; // "Li"
          case encode_16bit('B','e'): return   4; // "Be"
          case encode_16bit('B',' '): return   5; // "B "
          case encode_16bit('C',' '): return   6; // "C "
          case encode_16bit('N',' '): return   7; // "N "
          case encode_16bit('O',' '): return   8; // "O "
          case encode_16bit('F',' '): return   9; // "F "
          case encode_16bit('N','e'): return  10; // "Ne"
          case encode_16bit('N','a'): return  11; // "Na"
          case encode_16bit('M','g'): return  12; // "Mg"
          case encode_16bit('A','l'): return  13; // "Al"
          case encode_16bit('S','i'): return  14; // "Si"
          case encode_16bit('P',' '): return  15; // "P "
          case encode_16bit('S',' '): return  16; // "S "
          case encode_16bit('C','l'): return  17; // "Cl"
          case encode_16bit('A','r'): return  18; // "Ar"
          case encode_16bit('K',' '): return  19; // "K "
          case encode_16bit('C','a'): return  20; // "Ca"
          case encode_16bit('S','c'): return  21; // "Sc"
          case encode_16bit('T','i'): return  22; // "Ti"
          case encode_16bit('V',' '): return  23; // "V "
          case encode_16bit('C','r'): return  24; // "Cr"
          case encode_16bit('M','n'): return  25; // "Mn"
          case encode_16bit('F','e'): return  26; // "Fe"
          case encode_16bit('C','o'): return  27; // "Co"
          case encode_16bit('N','i'): return  28; // "Ni"
          case encode_16bit('C','u'): return  29; // "Cu"
          case encode_16bit('Z','n'): return  30; // "Zn"
          case encode_16bit('G','a'): return  31; // "Ga"
          case encode_16bit('G','e'): return  32; // "Ge"
          case encode_16bit('A','s'): return  33; // "As"
          case encode_16bit('S','e'): return  34; // "Se"
          case encode_16bit('B','r'): return  35; // "Br"
          case encode_16bit('K','r'): return  36; // "Kr"
          case encode_16bit('R','b'): return  37; // "Rb"
          case encode_16bit('S','r'): return  38; // "Sr"
          case encode_16bit('Y',' '): return  39; // "Y "
          case encode_16bit('Z','r'): return  40; // "Zr"
          case encode_16bit('N','b'): return  41; // "Nb"
          case encode_16bit('M','o'): return  42; // "Mo"
          case encode_16bit('T','c'): return  43; // "Tc"
          case encode_16bit('R','u'): return  44; // "Ru"
          case encode_16bit('R','h'): return  45; // "Rh"
          case encode_16bit('P','d'): return  46; // "Pd"
          case encode_16bit('A','g'): return  47; // "Ag"
          case encode_16bit('C','d'): return  48; // "Cd"
          case encode_16bit('I','n'): return  49; // "In"
          case encode_16bit('S','n'): return  50; // "Sn"
          case encode_16bit('S','b'): return  51; // "Sb"
          case encode_16bit('T','e'): return  52; // "Te"
          case encode_16bit('I',' '): return  53; // "I "
          case encode_16bit('X','e'): return  54; // "Xe"
          case encode_16bit('C','s'): return  55; // "Cs"
          case encode_16bit('B','a'): return  56; // "Ba"
          case encode_16bit('L','a'): return  57; // "La"
          case encode_16bit('C','e'): return  58; // "Ce"
          case encode_16bit('P','r'): return  59; // "Pr"
          case encode_16bit('N','d'): return  60; // "Nd"
          case encode_16bit('P','m'): return  61; // "Pm"
          case encode_16bit('S','m'): return  62; // "Sm"
          case encode_16bit('E','u'): return  63; // "Eu"
          case encode_16bit('G','d'): return  64; // "Gd"
          case encode_16bit('T','b'): return  65; // "Tb"
          case encode_16bit('D','y'): return  66; // "Dy"
          case encode_16bit('H','o'): return  67; // "Ho"
          case encode_16bit('E','r'): return  68; // "Er"
          case encode_16bit('T','m'): return  69; // "Tm"
          case encode_16bit('Y','b'): return  70; // "Yb"
          case encode_16bit('L','u'): return  71; // "Lu"
          case encode_16bit('H','f'): return  72; // "Hf"
          case encode_16bit('T','a'): return  73; // "Ta"
          case encode_16bit('W',' '): return  74; // "W "
          case encode_16bit('R','e'): return  75; // "Re"
          case encode_16bit('O','s'): return  76; // "Os"
          case encode_16bit('I','r'): return  77; // "Ir"
          case encode_16bit('P','t'): return  78; // "Pt"
          case encode_16bit('A','u'): return  79; // "Au"
          case encode_16bit('H','g'): return  80; // "Hg"
          case encode_16bit('T','l'): return  81; // "Tl"
          case encode_16bit('P','b'): return  82; // "Pb"
          case encode_16bit('B','i'): return  83; // "Bi"
          case encode_16bit('P','o'): return  84; // "Po"
          case encode_16bit('A','t'): return  85; // "At"
          case encode_16bit('R','n'): return  86; // "Rn"
          case encode_16bit('F','r'): return  87; // "Fr"
          case encode_16bit('R','a'): return  88; // "Ra"
          case encode_16bit('A','c'): return  89; // "Ac"
          case encode_16bit('T','h'): return  90; // "Th"
          case encode_16bit('P','a'): return  91; // "Pa"
          case encode_16bit('U',' '): return  92; // "U "
          case encode_16bit('N','p'): return  93; // "Np"
          case encode_16bit('P','u'): return  94; // "Pu"
          case encode_16bit('A','m'): return  95; // "Am"
          case encode_16bit('C','m'): return  96; // "Cm"
          case encode_16bit('B','k'): return  97; // "Bk"
          case encode_16bit('C','f'): return  98; // "Cf"
          case encode_16bit('E','s'): return  99; // "Es"
          case encode_16bit('F','m'): return 100; // "Fm"
          case encode_16bit('M','d'): return 101; // "Md"
          case encode_16bit('N','o'): return 102; // "No"
          case encode_16bit('L','r'): return 103; // "Lr"
          case encode_16bit('R','f'): return 104; // "Rf"
          case encode_16bit('D','b'): return 105; // "Db"
          case encode_16bit('S','g'): return 106; // "Sg"
          case encode_16bit('B','h'): return 107; // "Bh"
          case encode_16bit('H','s'): return 108; // "Hs"
          case encode_16bit('M','t'): return 109; // "Mt"
          case encode_16bit('D','s'): return 110; // "Ds"
          case encode_16bit('R','g'): return 111; // "Rg"
          case encode_16bit('C','n'): return 112; // "Cn"
          case encode_16bit('N','h'): return 113; // "Nh"
          case encode_16bit('F','l'): return 114; // "Fl"
          case encode_16bit('M','c'): return 115; // "Mc"
          case encode_16bit('L','v'): return 116; // "Lv"
          case encode_16bit('T','s'): return 117; // "Ts"
          case encode_16bit('O','g'): return 118; // "Og"
          case encode_16bit('u','e'): return 119; // "ue"
          case encode_16bit('u','0'): return 120; // "u0"
          case encode_16bit('u','1'): return 121; // "u1"
          case encode_16bit('u','2'): return 122; // "u2"
          case encode_16bit('u','3'): return 123; // "u3"
          case encode_16bit('u','4'): return 124; // "u4"
          case encode_16bit('u','5'): return 125; // "u5"
          case encode_16bit('u','6'): return 126; // "u6"
          case encode_16bit('e',' '): return 127; // "e "

          //   up to here, the code is auto-generated by test_consistency, see below
          case encode_16bit('-','1'): return 127; // "e "
          case encode_16bit('0',' '): return   0; // "__"
          case encode_16bit('_',' '): return   0; // "__"
          default:
          {
              if (allow_digit_reading) {
                  // try numeric reading, works only in [1, 99] since we have only 2 chars
                  if ((S >= allow_leading_zero ? '0' : '1') && (S <= '9')) {
                      // leading char is a digit [1, 9] or [0, 9] if leading zeros are allowed
                      if ((y >= '0') && (y <= '9')) {
                          // a 2-digit number
                          return (S - '0')*10 + (y - '0');
                      } else if ((y == 0) || (y == ' ')) {
                          // a single digit number, caution: case S='0' y=' ' is treated in the switch table above.
                          return (S - '0');
                      } // y
                  } // S
              } // allow_digit_reading

              // in priciple, the error return values could be different depending if there is
              // no element with that char S as first character or if the second char y does not exist for that S.
              return -128;
          } // default
      } // switch (code)
  } // decode

#ifdef  NO_UNIT_TESTS
  status_t all_tests(int const echo) { return STATUS_TEST_NOT_INCLUDED; }
#else // NO_UNIT_TESTS

  status_t test_consistency(int const echo=5) {
      // check that decode(ChemSymbol[Z]) == Z
      status_t stat(0);
      if (echo > 1) printf("\n# %s %s\n", __FILE__, __func__);
      for(int Z = 0; Z < 128; ++Z) {
          char const S = element_symbols[2*Z + 0], y = element_symbols[2*Z + 1];
          if (0) {
              // code-gen with explicit integers, needs to paste the generated code again if encode_16bit or element_symbols changes
              if (echo > 8) printf("          case %5i: return %3i; // \"%c%c\"\n", encode_16bit(S,y), Z, S,y);
          } else {
              // code-gen with constexpr function encode_16bit, needs to paste the generated code again only if element_symbols changes
              if (echo > 8) printf("          case encode_16bit(\'%c\',\'%c\'): return %3i; // \"%c%c\"\n", S,y, Z, S,y); // code-gen
          }
          int const failed = (decode(S, y) != Z);
          if (failed && echo > 1) printf("# %s: failed for Z=%i Sy=%c%c\n", __func__, Z, S,y); 
          stat += failed;
      } // Z
      if ((echo > 0) && (stat > 0)) printf("# %s %s failed for %i cases, run code generation again (verbosity > 8)!\n", __FILE__, __func__, stat);
      return stat;
  } // test_consistency

  status_t test_digit_reading(int const echo=5) {
      // check that decode(ChemSymbol[Z]) == Z
      status_t stat(0);
      if (echo > 1) printf("\n# %s %s\n", __FILE__, __func__);
      for(int iZ = -10; iZ < 100*int(allow_digit_reading); ++iZ) {
          bool const leading_zero = (iZ < 0); // the test cases [-10, -1] check if "09", ..., "00" are decoded correctly 
          int const Z = leading_zero ? 1 - iZ : iZ;
          char Sy[4];
          std::snprintf(Sy, 3, leading_zero ? "%2.2i" : "%i", Z);
          int const failed = (decode(Sy[0], Sy[1]) != Z);
          if (failed && echo > 1) printf("# %s: failed for Z=%i Sy=%s\n", __func__, Z, Sy); 
          stat += failed;
      } // Z
      return stat;
  } // test_digit_reading
  
  status_t all_tests(int const echo) {
      status_t stat(0);
      stat += std::abs(int(test_consistency(echo)));
      stat += std::abs(int(test_digit_reading(echo)));
      return stat;
  } // all_tests

#endif // NO_UNIT_TESTS

} // namespace chemical_symbol
