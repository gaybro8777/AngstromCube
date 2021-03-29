#pragma once

#include <cstdio> // printf, std::fflush, stdout
#include <cassert> // assert
#include <cstdint> // uint32_t --> replace size_t with this?
#include <algorithm> // std::fill
#include <utility> // std::move

#include "status.hxx" // status_t
#include "complex_tools.hxx" // conjugate

// #define debug_printf(...) printf(__VA_ARGS__)
#define debug_printf(...)

#ifdef DEVEL
  #ifndef NO_UNIT_TESTS
    #include "simple_timer.hxx" // SimpleTimer
  #endif
#endif

#define DimUnknown 0

template<typename T>
class view2D {
public:
  
  view2D() : _data(nullptr), _n0(DimUnknown), _n1(DimUnknown), _mem(0) { 
      // debug_printf("# view2D() default constructor\n");
  } // default constructor

  view2D(T* const ptr, size_t const stride) 
    : _data(ptr), _n0(stride), _n1(DimUnknown), _mem(0) { 
      // debug_printf("# view2D(ptr, stride=%i) constructor\n", stride);
  } // data view constructor

  view2D(size_t const n1, size_t const stride, T const init_value={0}) 
    : _data(new T[n1*stride]), _n0(stride), _n1(n1), _mem(n1*stride*sizeof(T)) {
      debug_printf("# view2D(n1=%i, stride=%i [, init_value]) constructor allocates %g kByte\n", n1, stride, _mem*.001);
      std::fill(_data, _data + n1*stride, init_value); // warning! first touch here!
  } // memory owning constructor, ToDo: move this to the derived type

  ~view2D() {
      // debug_printf("# ~view2D() destructor\n");
      if (_data && (_mem > 0)) {
          debug_printf("# ~view2D() destructor tries to free %g kByte\n", _mem*.001);
          delete[] _data;
      } // is memory owner
  } // destructor

  // view2D(view2D<T> && rhs) = delete;
  view2D(view2D<T> && rhs) {
      debug_printf("# view2D(view2D<T> && rhs);\n");
      *this = std::move(rhs);
  } // move constructor

  view2D(view2D<T> const & rhs) = delete;
  // view2D(view2D<T> const & rhs) {
  //     // copy constructor (may lead to problems, who owns the memory afterwards?)
  //     debug_printf("# view2D(view2D<T> const & rhs);\n");
  //     *this = rhs;
  // } // copy constructor

  view2D& operator= (view2D<T> && rhs) {
      debug_printf("# view2D& operator= (view2D<T> && rhs);\n");
      _data = rhs._data;
      _n0   = rhs._n0;
      _n1   = rhs._n1;
      _mem  = rhs._mem; rhs._mem = 0; // steal ownership
      return *this;
  } // move assignment

  view2D& operator= (view2D<T> const & rhs) = delete;
  // view2D& operator= (view2D<T> const & rhs) {
  //     debug_printf("# view2D& operator= (view2D<T> const & rhs);\n");
  //     _data = rhs._data;
  //     _n0   = rhs._n0; 
  //     _n1   = rhs._n1;
  //     _mem  = 0; // we are just a shallow copy
  //     return *this;
  // } // move assignment


#define _VIEW2D_HAS_PARENTHESIS
#ifdef  _VIEW2D_HAS_PARENTHESIS
  T const & operator () (size_t const i1, size_t const i0) const { return _data[i1*_n0 + i0]; } // (i,j)
  T       & operator () (size_t const i1, size_t const i0)       { return _data[i1*_n0 + i0]; } // (i,j)

  T const & at(size_t const i1, size_t const i0) const { assert(i0 < _n0); return _data[i1*_n0 + i0]; }
  T       & at(size_t const i1, size_t const i0)       { assert(i0 < _n0); return _data[i1*_n0 + i0]; }
#endif

  T* operator[] (size_t const i1) const {
    if    ((i1 < _n1) || (DimUnknown == _n1)) {} else printf("# i1=%li n1=%ld\n", i1, _n1);
    assert((i1 < _n1) || (DimUnknown == _n1)); 
    return &_data[i1*_n0]; } // []

  T* data() const { return _data; }
  size_t stride() const { return _n0; }

private:
  // private data members
  T * _data;
  size_t _n0, _n1; // _n1==0 --> unknown
  size_t _mem; // only > 0 if memory owner

}; // view2D


  template<typename T>
  inline void set(view2D<T> & y, size_t const n1, T const a) { 
          std::fill(y.data(), y.data() + n1*y.stride(), a); }

         
  template<typename T>
  view2D<T> transpose(view2D<T> const & a // input matrix a(N
      , int const aN // assume shape a(N,M)
      , int const aM=-1 // -1:auto use a.stride()
      , char const conj='n' // 'c':complex conjugate (for complex data types)
  ) {
      // define matrix-transposition a(N,M) --> a_transposed(M,N)

      int const N = (-1 == aM) ? a.stride() : aM;
      int const M = aN;
      assert( N <= a.stride() );
      bool const c = ('c' == (conj | 32)); // case insensitive: 'C' and 'c' lead to complex conjugation
      view2D<T> a_transposed(N, M);
      for(int n = 0; n < N; ++n) {
          for(int m = 0; m < M; ++m) {
              auto const a_mn = a(m,n); 
              a_transposed(n,m) = c ? conjugate(a_mn) : a_mn;
          } // m
      } // n
      return a_transposed;
  } // transpose

  template<typename Ta, typename Tb, typename Tc>
  void gemm(view2D<Tc> & c // result matrix, shape(N,M)
      , int const N, view2D<Tb> const & b //  left input matrix, shape(N,K)
      , int const K, view2D<Ta> const & a // right input matrix, shape(K,M)
      , int const aM=-1 // user specify M different from min(a.stride(), c.stride())
      , char const beta='0' // '0':overwrite elements of c, else add to c
  ) {
      // define a generic matrix-matrix multiplication: c(N,M) = b(N,K) * a(K,M)

      int const M = (-1 == aM) ? std::min(c.stride(), a.stride()) : aM;
      if (M > a.stride()) error("M= %d > %ld =a.stride", M, a.stride());
      if (K > b.stride()) error("K= %d > %ld =b.stride", M, b.stride());
      if (M > c.stride()) error("M= %d > %ld =c.stride", M, c.stride());
      assert( M <= a.stride() );
      assert( K <= b.stride() );
      assert( M <= c.stride() );
      for(int n = 0; n < N; ++n) {
          for(int m = 0; m < M; ++m) {
              Tc t(0);
              for(int k = 0; k < K; ++k) { // contraction index
                  t += b(n,k) * a(k,m);
              } // k
              if ('0' == beta) { c(n,m) = t; } else { c(n,m) += t; } // store
          } // m
      } // n
  } // gemm
         
         
         
template<typename T>
class view3D {
public:
  
  view3D() : _data(nullptr), _n0(0), _n1(0), _n2(DimUnknown), _mem(0) { } // default constructor

  view3D(T* const ptr, size_t const n1, size_t const stride)
    : _data(ptr), _n0(stride), _n1(n1), _n2(DimUnknown), _mem(0) { } // constructor

  view3D(size_t const n2, size_t const n1, size_t const stride, T const init_value={0}) 
    : _data(new T[n2*n1*stride]), _n0(stride), _n1(n1), _n2(n2), _mem(n2*n1*stride*sizeof(T)) {
      debug_printf("# view3D(n2=%i, n1=%i, stride=%i [, init_value]) constructor allocates %g kByte\n", n2, n1, stride, _mem*.001);
      std::fill(_data, _data + n2*n1*stride, init_value); // warning! first touch here!
  } // memory owning constructor

  ~view3D() { 
      if (_data && (_mem > 0)) {
          delete[] _data;
          debug_printf("# ~view3D() destructor tries to free %g kByte\n", _mem*.001);
      }
  } // destructor

  view3D(view3D<T>      && rhs) { 
      debug_printf("# view3D(view3D<T> && rhs);\n");
      *this = std::move(rhs);
  } // move constructor

  view3D(view3D<T> const & rhs) = delete; 
  // view3D(view3D<T> const & rhs) { 
  //     debug_printf("# view3D(view3D<T> const & rhs);\n");
  //     *this = rhs;
  // } // copy constructor

  // view3D& operator= (view3D<T> && rhs) = delete;
  view3D& operator= (view3D<T> && rhs) {
      debug_printf("# view3D& operator= (view3D<T> && rhs);\n");
      _data = rhs._data;
      _n0   = rhs._n0;
      _n1   = rhs._n1;
      _n2   = rhs._n2;
      _mem  = rhs._mem; rhs._mem = 0; // steal ownership
      return *this;
  } // move assignment

  view3D& operator= (view3D<T> const & rhs) = delete;
  // view3D& operator= (view3D<T> const & rhs) {
  //     debug_printf("# view3D& operator= (view3D<T> const & rhs);\n");
  //     _data = rhs._data;
  //     _n0   = rhs._n0;
  //     _n1   = rhs._n1;
  //     _n2   = rhs._n2;
  //     _mem  = 0; // we are just a shallow copy
  //     return *this;
  // } // move assignment
  
#define _VIEW3D_HAS_PARENTHESIS
#ifdef  _VIEW3D_HAS_PARENTHESIS
#define _access return _data[(i2*_n1 + i1)*_n0 + i0]
  T const & operator () (size_t const i2, size_t const i1, size_t const i0) const { _access; }
  T       & operator () (size_t const i2, size_t const i1, size_t const i0)       { _access; }

  T const & at(size_t const i2, size_t const i1, size_t const i0) const 
              { assert(i1 < _n1); assert(i0 < _n0); _access; }
  T       & at(size_t const i2, size_t const i1, size_t const i0)       
              { assert(i1 < _n1); assert(i0 < _n0); _access; }
#undef _access
#endif

#define _VIEW3D_HAS_PARENTHESIS_2ARGS
#ifdef  _VIEW3D_HAS_PARENTHESIS_2ARGS
#define _access return &_data[(i2*_n1 + i1)*_n0]
  T* const operator () (size_t const i2, size_t const i1) const { _access; }
  T*       operator () (size_t const i2, size_t const i1)       { _access; }

  T* const at(size_t const i2, size_t const i1) const { assert(i1 < _n1); _access; }
  T*       at(size_t const i2, size_t const i1)       { assert(i1 < _n1); _access; }
#undef _access
#endif

#define _VIEW3D_HAS_INDEXING
#ifdef  _VIEW3D_HAS_INDEXING
  view2D<T> operator[] (size_t const i2) const { return view2D<T>(_data + i2*_n1*_n0, _n0); } // [] returns a sub-array
  // maybe sub-optimal as it creates a view2D object every time
#endif

  T* data() const { return _data; }
  size_t stride() const { return _n0; }
  size_t dim1()   const { return _n1; }
  bool is_memory_owner() const { return (_n2 > DimUnknown); }

private:
  // private data members
  T* _data;
  size_t _n0, _n1, _n2; // _n2==0 -->unknown
  size_t _mem; // only > 0 if memory owner

}; // view3D

template<typename T>
inline void set(view3D<T> & y, size_t const n2, T const a) { 
         std::fill(y.data(), y.data() + n2*y.dim1()*y.stride(), a); }


template<typename T>
class view4D {
public:
  
  view4D() : _data(nullptr), _n0(0), _n1(0), _n2(0), _n3(DimUnknown) { } // default constructor

  view4D(T* const ptr, size_t const n2, size_t const n1, size_t const stride) 
    : _data(ptr), _n0(stride), _n1(n1), _n2(n2), _n3(DimUnknown) { } // constructor

  view4D(size_t const n3, size_t const n2, size_t const n1, size_t const stride, T const init_value={0}) 
    : _data(new T[n3*n2*n1*stride]), _n0(stride), _n1(n1), _n2(n2), _n3(n3), _mem(n3*n2*n1*stride*sizeof(T)) {
      debug_printf("# view4D(n3=%i, n2=%i, n1=%i, stride=%i [, init_value]) constructor allocates %g kByte\n", n3, n2, n1, stride, _mem*.001);
      std::fill(_data, _data + n3*n2*n1*stride, init_value); // warning! first touch here!
  } // memory owning constructor

  ~view4D() { 
      if (_data && (_mem > 0)) {
          delete[] _data;
          debug_printf("# ~view4D() destructor tries to free %g kByte\n", _mem*.001);
      }
  } // destructor

  view4D(view4D<T> && rhs) { 
      debug_printf("# view4D(view4D<T> && rhs);\n");
      *this = std::move(rhs);
  } // move constructor

  view4D(view4D<T> const & rhs) = delete;
  // view4D(view4D<T> const & rhs) { 
  //     debug_printf("# view4D(view4D<T> const & rhs);\n");
  //     *this = rhs;
  // } // copy constructor

  // view4D& operator= (view4D<T> && rhs) = delete;
  view4D& operator= (view4D<T> && rhs) {
      debug_printf("# view4D& operator= (view4D<T> && rhs);\n");
      _data = rhs._data;
      _n0   = rhs._n0;
      _n1   = rhs._n1;
      _n2   = rhs._n2;
      _n3   = rhs._n3;
      _mem  = rhs._mem; rhs._mem = 0; // steal ownership
      return *this;
  } // move assignment

  view4D& operator= (view4D<T> const & rhs) = delete;
  // view4D& operator= (view4D<T> const & rhs) {
  //     debug_printf("# view4D& operator= (view4D<T> const & rhs);\n");
  //     _data = rhs._data;
  //     _n0   = rhs._n0;
  //     _n1   = rhs._n1;
  //     _n2   = rhs._n2;
  //     _n3   = rhs._n3;
  //     _mem  = 0; // we are just a shallow copy
  //     return *this;
  // } // move assignment
  
#define _VIEW4D_HAS_PARENTHESIS
#ifdef  _VIEW4D_HAS_PARENTHESIS
#define _access return _data[((i3*_n2 + i2)*_n1 + i1)*_n0 + i0]
  T const & operator () (size_t const i3, size_t const i2, size_t const i1, size_t const i0) const { _access; }
  T       & operator () (size_t const i3, size_t const i2, size_t const i1, size_t const i0)       { _access; }

  T const & at(size_t const i3, size_t const i2, size_t const i1, size_t const i0) const 
              { assert(i2 < _n2); assert(i1 < _n1); assert(i0 < _n0); _access; }
  T       & at(size_t const i3, size_t const i2, size_t const i1, size_t const i0)       
              { assert(i2 < _n2); assert(i1 < _n1); assert(i0 < _n0); _access; }
#undef _access
#endif

#define _VIEW4D_HAS_PARENTHESIS_3ARGS
#ifdef  _VIEW4D_HAS_PARENTHESIS_3ARGS
#define _access return &_data[((i3*_n2 + i2)*_n1 + i1)*_n0]
  T* const operator () (size_t const i3, size_t const i2, size_t const i1) const { _access; }
  T*       operator () (size_t const i3, size_t const i2, size_t const i1)       { _access; }

  T* const at(size_t const i3, size_t const i2, size_t const i1) const 
            { assert(i2 < _n2); assert(i1 < _n1); _access; }
  T*       at(size_t const i3, size_t const i2, size_t const i1)
            { assert(i2 < _n2); assert(i1 < _n1); _access; }
#undef _access
#endif

#define _VIEW4D_HAS_PARENTHESIS_2ARGS
#ifdef  _VIEW4D_HAS_PARENTHESIS_2ARGS
  view2D<T> operator () (size_t const i3, size_t const i2) const {
      return view2D<T>(_data + (i3*_n2 + i2)*_n1*_n0, _n0); }
#endif

#define _VIEW4D_HAS_INDEXING
#ifdef  _VIEW4D_HAS_INDEXING
  view3D<T> operator[] (size_t const i3) const { return view3D<T>(_data + i3*_n2*_n1*_n0, _n1, _n0); } // [] returns a sub-array
  // maybe sub-optimal as it creates a view3D object every time
#endif

  T* data() const { return _data; }
  size_t stride() const { return _n0; }
  size_t dim1()   const { return _n1; }
  size_t dim2()   const { return _n2; }
  bool is_memory_owner() const { return (_n3 > DimUnknown); }

private:
  // private data members
  T* _data;
  size_t _n0, _n1, _n2, _n3; // _n3==0 -->unknown
  size_t _mem; // only > 0 if memory owner

}; // view4D

template<typename T>
inline void set(view4D<T> & y, size_t const n3, T const a) { 
         std::fill(y.data(), y.data() + n3*y.dim2()*y.dim1()*y.stride(), a); }

#undef DimUnknown

namespace data_view {

#ifdef  NO_UNIT_TESTS
  inline status_t all_tests(int const echo=0) { return STATUS_TEST_NOT_INCLUDED; }
#else // NO_UNIT_TESTS

  inline int test_view2D(int const echo=9) {
      int constexpr n1 = 3, n0 = 5;
      if (echo > 0) printf("\n# %s(%i,%i)\n", __func__, n1, n0);

      // view2D<double> a(n1,8); // memory allocation
      auto a = view2D<double>(n1,8);
      assert(a.stride() >= n0);
      for(int i = 0; i < n1; ++i) {
          for(int j = 0; j < n0; ++j) {
              #ifdef _VIEW2D_HAS_PARENTHESIS
              a(i,j) = i + 0.1*j;
              if (echo > 0) printf("# a2D(%i,%i) = %g\n", i, j, a(i,j));
              assert(a.at(i,j) == a[i][j]);
              #else
              a[i][j] = i + 0.1*j;
              if (echo > 0) printf("# a2D(%i,%i) = %g\n", i, j, a[i][j]);
              #endif
          } // j
      } // i

      int ii = 1;
      if (echo > 0) printf("\n# ai = a1D[%i][:]\n", ii);
      auto const ai = a[ii]; // pointer into contiguous memory
      for(int j = 0; j < n0; ++j) {
          if (echo > 0) printf("# ai[%i] = %g\n", j, ai[j]);
          #ifdef _VIEW2D_HAS_PARENTHESIS
          assert(a.at(ii,j) == ai[j]);
          #else
          assert(a[ii][j] == ai[j]);
          #endif
      } // j

      return 0;
  } // test_view2D

  inline int test_view3D(int const echo=9) {
      int constexpr n2 = 3, n1 = 2, n0 = 5;
      if (echo > 0) printf("\n# %s(%i,%i,%i)\n", __func__, n2, n1, n0);
      view3D<double> a(n2,n1,8); // memory allocation
      assert(a.stride() >= n0);
      for(int h = 0; h < n2; ++h) {
        for(int i = 0; i < n1; ++i) {
          for(int j = 0; j < n0; ++j) {
              a(h,i,j) = h + 0.1*i + 0.01*j;
              if (echo > 0) printf("# a3D(%i,%i,%i) = %g\n", h, i, j, a(h,i,j));
              #ifdef _VIEW3D_HAS_INDEXING
              assert(a(h,i,j) == a[h][i][j]);
              #endif
          } // j
        } // h
      } // i

      return 0;
  } // test_view3D

  inline status_t test_bench_view2D(int const echo=1) {
#ifdef DEVEL
      if (echo < 1) return 0;
      view2D<int> a(2, 2, 0);
      int const nrep = 1e7; // int(control::get("data_view.bench.nrepeat", 2e7));
      {   SimpleTimer t(__FILE__, __LINE__, "a[i][j]");
          for(int irep = 0; irep < nrep; ++irep) {
              a[1][1] = a[1][0];
              a[1][0] = a[0][1];
              a[0][1] = a[0][0];
              a[0][0] = irep;
          } // irep
      } // timer
      printf("# a[i][j] = %i %i %i %i\n", a(0,0), a(0,1), a(1,0), a(1,1));
      std::fflush(stdout);
      {   SimpleTimer t(__FILE__, __LINE__, "a(i,j)");
          for(int irep = 0; irep < nrep; ++irep) {
              a(1,1) = a(1,0);
              a(1,0) = a(0,1); // This version is 1.5x faster
              a(0,1) = a(0,0);
              a(0,0) = irep;
          } // irep
      } // timer
      printf("# a(i,j)  = %i %i %i %i\n", a(0,0), a(0,1), a(1,0), a(1,1));
      std::fflush(stdout);
#endif
      return 0;
  } // test_bench_view2D

  inline status_t all_tests(int const echo=0) {
      status_t status = 0;
      status += test_view2D(echo);
      status += test_view3D(echo);
      status += test_bench_view2D(echo);
      return status;
  } // all_tests

#endif // NO_UNIT_TESTS  

} // namespace data_view

#undef debug_printf