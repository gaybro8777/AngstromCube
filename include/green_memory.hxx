#pragma once
// This file is part of AngstromCube under MIT License

#ifndef   HAS_NO_CUDA

  #include <cuda.h> // dim3, cudaStream_t, __syncthreads, cuda*

  __host__ inline
  void __cudaSafeCall(cudaError_t err, char const *file, int const line, char const *call) { // Actual check function
      if (cudaSuccess != err) {
          auto const msg = cudaGetErrorString(err);
          std::fprintf(stdout, "[ERROR] CUDA call to %s at %s:%d\n%s\n", call, file, line, msg); std::fflush(stdout);
          std::fprintf(stderr, "[ERROR] CUDA call to %s at %s:%d\n%s\n", call, file, line, msg); std::fflush(stderr);
          std::exit(line);
      }
  } // __cudaSafeCall
  #define cuCheck(err) __cudaSafeCall((err), __FILE__, __LINE__, #err) // Syntactic sugar to enhance output

#else  // HAS_NO_CUDA

  // replace CUDA specifics
  #define __global__
  #define __restrict__
  #define __device__
  #define __shared__
  #define __unroll__
  #define __host__

  struct dim3 {
      int x, y, z;
      dim3(int xx, int yy=1, int zz=1) : x(xx), y(yy), z(zz) {}
  }; // dim3

#ifndef   HAS_TFQMRGPU
    inline void __syncthreads(void) {} // dummy
    typedef int cudaError_t;
    inline cudaError_t cudaDeviceSynchronize(void) { return 0; } // dummy
    inline cudaError_t cudaPeekAtLastError(void) { return 0; } // dummy
#else  // HAS_TFQMRGPU
    #define gpuStream_t cudaStream_t
    #include "tfQMRgpu/include/tfqmrgpu_cudaStubs.hxx" // cuda... (dummies)
#endif // HAS_TFQMRGPU

  #define cuCheck(err) ;

#endif // HAS_NO_CUDA

// #define DEBUG

  template <typename T>
  T* get_memory(size_t const size=1, int const echo=0, char const *const name="") {

#ifdef    DEBUG
      if (echo > 0) {
          size_t const total = size*sizeof(T);
          std::printf("# managed memory: %lu x %.3f kByte = \t", size, sizeof(T)*1e-3);
          if (total > 1e9) { std::printf("%.9f GByte\n", total*1e-9); } else 
          if (total > 1e6) { std::printf("%.6f MByte\n", total*1e-6); } else 
                           { std::printf("%.3f kByte\n", total*1e-3); }
      } // echo
#endif // DEBUG

      T* ptr{nullptr};
#ifndef HAS_NO_CUDA
      cuCheck(cudaMallocManaged(&ptr, size*sizeof(T)));
#else  // HAS_NO_CUDA
      ptr = new T[size];
#endif // HAS_NO_CUDA

#ifdef    DEBUGGPU
      std::printf("# get_memory \t%lu x %.3f kByte = \t%.3f kByte, %s at %p\n", size, sizeof(T)*1e-3, size*sizeof(T)*1e-3, name, (void*)ptr);
#endif // DEBUGGPU

      return ptr;
  } // get_memory


  template <typename T>
  void _free_memory(T* & ptr, char const *const name="") {
//    std::printf("# free_memory %s at %p\n", name, (void*)ptr);
      if (nullptr != ptr) {
#ifdef    DEBUGGPU
          std::printf("# free_memory %s at %p\n", name, (void*)ptr);
#endif // DEBUGGPU

#ifndef HAS_NO_CUDA
          cuCheck(cudaFree((void*)ptr));
#else  // HAS_NO_CUDA
          delete[] ptr;
#endif // HAS_NO_CUDA
      } // d
      ptr = nullptr;
  } // free_memory

#define free_memory(PTR) _free_memory(PTR, #PTR)

  template <typename real_t=float>
  inline char const* real_t_name() { return (8 == sizeof(real_t)) ? "double" : "float"; }

//   template <typename result_t, typename input_t>
//   void safe_assign(result_t & result, input_t const & input) {
//       result = input; // assign
//       assert(input == result);
//   } // safe_assign

  //
  // Memory layout for Green function and atomic projection coefficients
  //
  //  version G(*)[R1C2][Noco*64][Noco*64]
  //          a(*)[R1C2][Noco   ][Noco*64]
  //
  //            kinetic:    <<< {16, Nrows, 1}, {Noco*64, Noco, R1C2} >>>
  //            add:        <<< {nrhs, ncubes, 1}, {Noco*64, 1, 1} >>>
  //            prj:        <<< {nrhs, natoms, 1}, {Noco*64, 1, 1} >>>
  //            potential:  <<< {64, any, 1}, {Noco*64, Noco, R1C2} >>>
  //  --> if 2 == R1C2, tfQMRgpu with LM=Noco*64
  //
