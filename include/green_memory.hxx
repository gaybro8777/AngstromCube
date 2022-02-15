#pragma once

#ifdef HAS_NO_CUDA
  struct dim3 {
      int x, y, z;
      dim3(int xx, int yy=1, int zz=1) : x(xx), y(yy), z(zz) {}
  }; // dim3

  inline void __syncthreads(void) {} // dummy
#endif // HAS_NO_CUDA      

  template <typename T>
  T* get_memory(size_t const size=1, int const echo=0) {
#ifdef DEBUG
      double const GByte = 1e-9; 
      char const *const _GByte = "GByte";
      if (echo > 0) std::printf("# managed memory: %lu x %.3f kByte = \t%.6f %s\n", size, 1e-3*sizeof(T), size*sizeof(T)*GByte, _GByte);
#endif // DEBUG
      T* d{nullptr};
#ifdef  HAS_CUDA
      CCheck(cudaMallocManaged(&d, size*sizeof(T)));
#else  // HAS_CUDA
      d = new T[size];
#endif // HAS_CUDA
      return d;
  } // get_memory

  template <typename T>
  void free_memory(T* &d) {
      if (nullptr != d) {
#ifdef  HAS_CUDA
          CCheck(cudaFree(d));
#else  // HAS_CUDA
          delete[] d;
#endif // HAS_CUDA
      } // d
      d = nullptr;
  } // free_memory

  
  //
  // Memory layout for Green function and atomic projection coefficients
  //
  //  version G(*)[R1C2][Noco*64][Noco*64]
  //          a(*)[R1C2][Noco   ][Noco*64]
  //
  //            kinetic:    <<< {16, Nrows, 1}, {Noco*64, Noco, R1C2} >>>
  //            add:        <<< {nRHSs/(Noco*64), ncubes, 1}, {Noco*64, 1, 1} >>>
  //            prj:        <<< {nRHSs/(Noco*64), natoms, 1}, {Noco*64, 1, 1} >>>
  //            potential:  <<< {64, any, 1}, {Noco*64, Noco, R1C2} >>>
  //  --> if 2 == R1C2, tfQMRgpu with LM=Noco*64
  //
  //  proposed
  //  version G(*)[Noco*Noco*R1C2][64][64]
  //  version a(*)[Noco*Noco*R1C2]    [64]
  //            --> add/prj kernels do not need templating except for real_t
  //  --> if 2 == R1C2, tfQMRgpu with LM=64, only 1 == Noco possible
  //
  //  proposed
  //  version G(*)[64][Noco*Noco*R1C2][64]
  //  version a(*)    [Noco*Noco*R1C2][64]
  //            --> better for intuition, tfQMRgpu cannot be used
  //
