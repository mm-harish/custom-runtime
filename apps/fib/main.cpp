// ================= main.cpp =================
#include "fib.h"
#include "runtime.h"
#include <chrono>

int FINAL_RESULT = 0;

int main(int argc, char **argv) {
  int numThreads = 8;
  bool validateRequested = false;
  if (argc > 1) {
    std::string arg1 = argv[1];
    if (arg1 == "--validate") {
      validateRequested = true;
      if (argc > 2)
        numThreads = atoi(argv[2]);
    } else {
      numThreads = atoi(argv[1]);
      if (argc > 2 && std::string(argv[2]) == "--validate") {
        validateRequested = true;
      }
    }
  }

  auto start = std::chrono::high_resolution_clock::now();
  Runtime<Worker<FuncType>> rt(numThreads);
  rt.run();
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end - start;
  std::cout << "Computation Time: " << elapsed.count() << " seconds\n";
  std::cout << "sum:" << FINAL_RESULT << "\n";

#ifdef PERFORM_VALIDATION
  if (validateRequested) {
    int expected = serial_fib(FIB_INPUT);
    if (FINAL_RESULT == expected) {
      std::cout << "Validation Passed\n";
    } else {
      std::cout << "Validation Failed: Expected " << expected << ", got "
                << FINAL_RESULT << "\n";
    }
  }
#else
  if (validateRequested) {
    std::cout << "Validation Disabled (compiled out)\n";
  }
#endif

  return 0;
}
