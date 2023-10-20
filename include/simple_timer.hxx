#pragma once
// This file is part of AngstromCube under MIT License

#include <cstdio> // std::printf
#include <chrono> // std::chrono::high_resolution_clock
#include <string> // std::string

#include "status.hxx" // status_t
#ifndef NO_UNIT_TESTS
  #include <vector> // std::vector<>
  #include <cstring> // std::strcmp
  #include "simple_stats.hxx" // ::Stats<T>
#endif // NO_UNIT_TESTS

inline char const * strip_path(char const *const str=nullptr, char const search='/') {
    if (nullptr == str) return "";
    char const *pt = str;
    for (char const *c = str; *c != '\0'; ++c) {
        if (*c == search) { pt = c + 1; }
    } // c
    return pt;
} // strip_path

  class SimpleTimer {
    // This timer object prints the time elapsed between construction and destruction 
    // into stdout immediately when destructed. See test_usage() below.
    // When stop() is called, the timer returns the elapsed time in seconds as double.
    private:
      std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
      std::string file;
      std::string func;
      int line;
      int echo;
    public:

      SimpleTimer(char const *sourcefile, int const sourceline=0, char const *function=nullptr, int const echo=2)
      : file(strip_path(sourcefile)), func(function), line(sourceline), echo(echo) {
          start_time = std::chrono::high_resolution_clock::now(); // start
      } // constructor

      double stop(int const stop_echo=0) const { 
          auto const stop_time = std::chrono::high_resolution_clock::now(); // stop
          auto const musec = std::chrono::duration_cast<std::chrono::microseconds>(stop_time - start_time).count();
          if (stop_echo > 0) std::printf("# timer started at %s:%d %s took %.5f sec\n", file.c_str(), line, func.c_str(), 1e-6*musec);
          return 1e-6*musec;
      } // stop

      ~SimpleTimer() { stop(echo); } // destructor

  }; // class SimpleTimer







namespace simple_timer {

#ifdef  NO_UNIT_TESTS
  inline status_t all_tests(int const echo=0) { return STATUS_TEST_NOT_INCLUDED; }
#else // NO_UNIT_TESTS

  inline status_t test_strip_path(int const echo=0) {
      status_t stat(0);
      stat += std::strcmp(strip_path("///"), "");
      stat += std::strcmp(strip_path("/full/path/nowhere/"), "");
      stat += std::strcmp(strip_path("/full/path/somewhere"), "somewhere");
      stat += std::strcmp(strip_path("/very/long//../path/to"), "to");
      if (echo > 0) std::printf("# %s: %d errors\n", __func__, int(stat));
      return stat;
  } // test_strip_path

  inline int64_t fibonacci(int64_t const n) {
      if (n < 3) return (n > 0);
      return fibonacci(n - 1) + fibonacci(n - 2);
  } // fibonacci

  inline int64_t fibonacci_nonrecursive(int64_t const n) {
      if (n < 3) return (n > 0);
      std::vector<int64_t> fib(n + 1);
      fib[0] = 0;
      fib[1] = 1;
      for (int i = 2; i <= n; ++i) {
          fib[i] = fib[i - 1] + fib[i - 2];
      } // i
      return fib[n];
  } // fibonacci (fast)

  inline status_t test_basic_usage(int const echo=3) {
      int64_t result;
      int64_t const inp = 40, reference = fibonacci_nonrecursive(inp);
      { // scope: create a timer, do some work, destroy the timer
          SimpleTimer timer(__FILE__, __LINE__, "comment=fibonacci", echo);
          result = fibonacci(inp);
          // timer destructor is envoked at the end of this scope, timing printed to log
      } // scope
      if (echo > 0) std::printf("# fibonacci(%lld) = %lld\n", inp, result);
      return (reference != result);
  } // test_basic_usage

  inline status_t test_stop_function(int const echo=3) {
      status_t stat(0);
      int64_t result;
      simple_stats::Stats<> s;
      for (int inp = 40; inp < 45; ++inp) {
          auto const reference = fibonacci_nonrecursive(inp);
          SimpleTimer timer(__FILE__, __LINE__, "", 0);
          result = fibonacci(inp);
          stat += (reference != result);
          if (echo > 7) std::printf("# fibonacci(%lld) = %lld\n", inp, result);
          s.add(timer.stop());
      } // scope
      auto const average_time = s.mean();
      if (echo > 2) std::printf("# fibonacci took %g +/- %.1e seconds per iteration, %g seconds in total\n", average_time, s.dev(), s.sum());
      return stat + (average_time < 0);
  } // test_stop_function

  inline status_t all_tests(int const echo=0) {
      if (echo > 2) std::printf("\n# %s %s\n\n", __FILE__, __func__);
      status_t stat(0);
      stat += test_strip_path(echo);
      stat += test_basic_usage(echo);
      stat += test_stop_function(echo);
      return stat;
  } // all_tests

#endif // NO_UNIT_TESTS

} // namespace simple_timer
