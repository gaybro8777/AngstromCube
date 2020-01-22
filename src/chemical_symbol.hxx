#pragma once

#include <cstdint> // int8_t

typedef int status_t;

namespace chemical_symbol {

  int8_t decode(char const S, char const y);
  status_t get(int const Z, char* Sy);
  
  status_t all_tests();

} // namespace chemical_symbol
