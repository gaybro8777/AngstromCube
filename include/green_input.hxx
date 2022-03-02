#pragma once

#include <cassert> // assert
#include <vector> // std::vector<T>
#include <cstdio> // std::printf
#include <cstdlib> // std::atoi, ::atof

#include "status.hxx" // status_t

#ifdef  HAS_RAPIDXML
  // git clone https://github.com/dwd/rapidxml
  #include "rapidxml/rapidxml.hpp" // ::xml_document<>
  #include "rapidxml/rapidxml_utils.hpp" // rapidxml::file<>

  #include "xml_reading.hxx" // ::find_attribute, ::find_child
#endif // HAS_RAPIDXML

#include "sho_tools.hxx" // ::nSHO
#include "xml_reading.hxx" // ::read_sequence
#include "inline_math.hxx" // set

namespace green_input {

  inline status_t load_Hamiltonian(
        int ng[3] // number of grid points
      , double hg[3] // grid spacing
      , std::vector<double> & Veff
      , int & natoms
      , std::vector<double> & xyzZinso
      , std::vector<std::vector<double>> & atom_mat
      , int const echo=0 // log-level
  ) {
#ifndef HAS_RAPIDXML
      warn("Unable to load_Hamiltonian when compiled without -D HAS_RAPIDXML", 0);
      return STATUS_TEST_NOT_INCLUDED;
#else  // HAS_RAPIDXML
      char const *filename = "Hmt.xml";
      rapidxml::file<> infile(filename);

      rapidxml::xml_document<> doc;
      doc.parse<0>(infile.data());

      set(ng, 3, 0);
      set(hg, 3, 1.0);
      Veff.resize(0);
      natoms = 0;
      xyzZinso.resize(0);
      atom_mat.resize(0);

      auto const grid_Hamiltonian = doc.first_node("grid_Hamiltonian");
      if (grid_Hamiltonian) {
          auto const sho_atoms = xml_reading::find_child(grid_Hamiltonian, "sho_atoms", echo);
          if (sho_atoms) {
              auto const number = xml_reading::find_attribute(sho_atoms, "number", "0", echo);
              if (echo > 5) std::printf("# found number=%s\n", number);
              natoms = std::atoi(number);
              xyzZinso.resize(natoms*8);
              atom_mat.resize(natoms);
              int ia{0};
              for (auto atom = sho_atoms->first_node(); atom; atom = atom->next_sibling()) {
                  auto const gid = xml_reading::find_attribute(atom, "gid", "-1");
                  if (echo > 5) std::printf("# <%s gid=%s>\n", atom->name(), gid);
                  xyzZinso[ia*8 + 4] = std::atoi(gid);

                  double pos[3] = {0, 0, 0};
                  auto const position = xml_reading::find_child(atom, "position", echo);
                  for (int d = 0; d < 3; ++d) {
                      char axyz[] = {0, 0}; axyz[0] = 'x' + d; // "x", "y", "z"
                      auto const value = xml_reading::find_attribute(position, axyz);
                      if (*value != '\0') {
                          pos[d] = std::atof(value);
                          if (echo > 5) std::printf("# %s = %.15g\n", axyz, pos[d]);
                      } else warn("no attribute '%c' found in <atom><position> in file %s", *axyz, filename);
                      xyzZinso[ia*8 + d] = pos[d];
                  } // d

                  auto const projectors = xml_reading::find_child(atom, "projectors", echo);
                  int numax{-1};
                  {
                      auto const value = xml_reading::find_attribute(projectors, "numax", "-1");
                      if (*value != '\0') {
                          numax = std::atoi(value);
                          if (echo > 5) std::printf("# numax= %d\n", numax);
                      } else warn("no attribute 'numax' found in <projectors> in file %s", filename);
                  }
                  double sigma{-1};
                  {
                      auto const value = xml_reading::find_attribute(projectors, "sigma", "");
                      if (*value != '\0') {
                          sigma = std::atof(value);
                          if (echo > 5) std::printf("# sigma= %g\n", sigma);
                      } else warn("no attribute 'sigma' found in <projectors> in file %s", filename);
                  }
                  xyzZinso[ia*8 + 5] = numax;
                  xyzZinso[ia*8 + 6] = sigma;
                  int const nSHO = sho_tools::nSHO(numax);
                  atom_mat[ia].resize(2*nSHO*nSHO);
                  for (int h0s1 = 0; h0s1 < 2; ++h0s1) {
                      auto const matrix_name = h0s1 ? "overlap" : "hamiltonian";
                      auto const matrix = xml_reading::find_child(atom, matrix_name, echo);
                      if (matrix) {
                          if (echo > 22) std::printf("# %s.values= %s\n", matrix_name, matrix->value());
                          auto const v = xml_reading::read_sequence<double>(matrix->value(), echo, nSHO*nSHO);
                          if (echo > 5) std::printf("# %s matrix has %ld values, expect %d x %d = %d\n",
                              matrix_name, v.size(), nSHO, nSHO, nSHO*nSHO);
                          assert(v.size() == nSHO*nSHO);
                          set(atom_mat[ia].data() + h0s1*nSHO*nSHO, nSHO*nSHO, v.data()); // copy
                      } else warn("atom with global_id=%s has no %s matrix!", gid, matrix_name);
                  } // h0s1
                  ++ia; // count up the number of atoms
              } // atom
              assert(natoms == ia); // sanity
          } else warn("no <sho_atoms> found in grid_Hamiltonian in file %s", filename);

          auto const spacing = xml_reading::find_child(grid_Hamiltonian, "spacing", echo);
          for (int d = 0; d < 3; ++d) {
              char axyz[] = {0, 0}; axyz[0] = 'x' + d; // "x", "y", "z"
              auto const value = xml_reading::find_attribute(spacing, axyz);
              if (*value != '\0') {
                  hg[d] = std::atof(value);
                  if (echo > 5) std::printf("# h%s = %.15g\n", axyz, hg[d]);
              } // value != ""
          } // d

          auto const potential = xml_reading::find_child(grid_Hamiltonian, "potential", echo);
          if (potential) {
              for (int d = 0; d < 3; ++d) {
                  char axyz[] = {'n', 0, 0}; axyz[1] = 'x' + d; // "nx", "ny", "nz"
                  auto const value = xml_reading::find_attribute(potential, axyz);
                  if (*value != '\0') {
                      ng[d] = std::atoi(value);
                      if (echo > 5) std::printf("# %s = %d\n", axyz, ng[d]);
                  } // value != ""
              } // d
              if (echo > 33) std::printf("# potential.values= %s\n", potential->value());
              Veff = xml_reading::read_sequence<double>(potential->value(), echo, ng[2]*ng[1]*ng[0]);
              if (echo > 5) std::printf("# potential has %ld values, expect %d x %d x %d = %d\n",
                  Veff.size(), ng[0], ng[1], ng[2], ng[2]*ng[1]*ng[0]);
              assert(Veff.size() == ng[2]*ng[1]*ng[0]);
          } else warn("grid_Hamiltonian has no potential in file %s", filename);

      } else warn("no grid_Hamiltonian found in file %s", filename);

      return 0;
#endif // HAS_RAPIDXML
  } // load_Hamiltonian

} // namespace green_input