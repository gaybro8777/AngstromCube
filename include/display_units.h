#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _Output_Units_Fixed
  double const eV  = 1; char const _eV[] = "Ha"; // Hartree atomic energy unit
  double const Ang = 1; char const _Ang[] = "Bohr"; // Bohr atomic length unit
#else
  extern double eV;  extern char const *_eV;  // dynamic energy unit
  extern double Ang; extern char const *_Ang; // dynamic length unit
#endif
  double const Kelvin = 315773.244215; char const _Kelvin[] = "Kelvin";

#ifdef __cplusplus
} // extern "C"
#endif