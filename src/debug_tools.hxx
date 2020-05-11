#pragma once

#include <cstdio> // printf
#include <cassert> // assert
#include <fstream> // std::fstream
#include <sstream> // std::sstream
#include <string> // std::string

namespace debug_tools {

  template<typename real_t>
  int read_from_file(
    real_t y_data[], // 2D-data array [N*Stride]
    char const *filename,
    int const N, // expected number of rows
    int const Stride=1, // Stride, default=1 for 1D functions
    int const M=1, // number of columns to fill
    char const *title=nullptr, // title (only for display to log)
    int const echo=0) // report writing to stdout, 0: suppress report
  {
      assert(M <= Stride);
      std::ifstream infile(filename, std::ifstream::in);
      if (infile.fail()) {
          if (echo > 1) printf("# %s Error opening file %s!\n", __func__, filename);
          return 1;
      } // failed to open

      std::string line; // read ASCII file line-by-line
      unsigned linenumber = 1;
      int ix = 0;
      while (std::getline(infile, line)) {
          char const *line_ptr = line.c_str();
          char const c0 = line_ptr ? *line_ptr : '\0';
          switch (c0) {
              case '#': {
                  if (echo > 3) printf("# %s:%d reads \'%s\'.\n", filename, linenumber, line.c_str());
              } break;
              case ' ': case '\n': case '\t': case '\0': {
                  if (echo > 9) printf("# %s:%d reads \'%s\'.\n", filename, linenumber, line.c_str());
              } break;
              default: {
                  std::istringstream iss(line);
                  double x{0};
                  iss >> x;
                  for (int iy = 0; iy < M; ++iy) {
                      double y{0};
                      iss >> y;
                      if (ix < N) y_data[ix*Stride + iy] = y;
                  } // iy
                  ++ix;
              } // default
          } // switch
          ++linenumber;
      } // while
      if (echo > 3) printf("# %d (of %d) x %d (of %d) data entries%s%s read from file %s.\n", 
                              ix, N, M, Stride, title?" for ":"", title, filename);
      return ix - N; // 0 if number of lines + 1 matched the expected number of rows
  } // read_from_file

} // namespace debug_tools