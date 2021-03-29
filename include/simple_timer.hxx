#pragma once

#include <chrono> // std::chrono::high_resolution_clock
#include <cstring> // std::strcpy
#include <cstdio> // std::printf

#include "status.hxx" // status_t
#ifndef NO_UNIT_TESTS
  #include "simple_stats.hxx" // Stats<T>
#endif

  class SimpleTimer {
    // This timer object prints the time elapsed between construction and destruction 
    // into stdout immediately when destructed. See test_usage() below.
    // When stop() is called, the timer returns the elapsed time in seconds as double.
    private:
      std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
      char file[60];
      int  line;
      char func[32];
      int  echo;
    public:

      SimpleTimer(char const *sourcefile, int const sourceline=0, char const *function=nullptr, int const echo=1) 
      : line(sourceline), echo(echo) {
          std::strcpy(file, sourcefile);
          if (nullptr != function) std::strcpy(func, function); else func[0] = 0;
          start_time = std::chrono::high_resolution_clock::now(); // start
      } // constructor

      double stop(int const stop_echo=0) const { 
          auto const stop_time = std::chrono::high_resolution_clock::now(); // stop
          auto const musec = std::chrono::duration_cast<std::chrono::microseconds>(stop_time - start_time).count();
          if (stop_echo > 0) std::printf("# timer started at %s:%d %s took %.5f sec\n", file, line, func, 1e-6*musec);
          return 1e-6*musec;
      } // stop

      ~SimpleTimer() { stop(echo); } // destructor

  }; // class SimpleTimer
  
namespace simple_timer {

#ifdef  NO_UNIT_TESTS
  inline status_t all_tests(int const echo=0) { return STATUS_TEST_NOT_INCLUDED; }
#else // NO_UNIT_TESTS

  inline int64_t fibonacci(int64_t const n) { if (n < 3) return 1; return fibonacci(n - 1) + fibonacci(n - 2); }

  inline status_t test_usage(int const echo=3) {
      int64_t result, inp = 40;
      { // scope: create a timer, do some work, destroy the timer
          SimpleTimer timer(__FILE__, __LINE__, "comment=fibonacci", echo);
          result = fibonacci(inp);
          // timer destructor is envoked at the end of this scope
      } // scope
      if (echo > 0) std::printf("# fibonacci(%lld) = %lld\n", inp, result);
      return 0;
  } // test_usage

  inline status_t test_stop_function(int const echo=3) {
      int64_t result, inp = 40;
      simple_stats::Stats<> s;
      for(int i = 0; i < 5; ++i) {
          SimpleTimer timer(__FILE__, __LINE__, "", 0);
          result = fibonacci(inp);
          s.add(timer.stop());
          // timer destructor is envoked at the end of this scope
      } // scope
      if (echo > 0) std::printf("# fibonacci(%lld) = %lld took %g +/- %g seconds\n", inp, result, s.avg(), s.var());
      return 0;
  } // test_stop_function

  inline status_t all_tests(int const echo=0) {
      if (echo > 0) std::printf("\n# %s %s\n", __FILE__, __func__);
      status_t status(0);
      status += test_usage(echo);
      status += test_stop_function(echo);
      return status;
  } // all_tests

#endif // NO_UNIT_TESTS  

} // namespace simple_timer