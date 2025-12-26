// ================= main.cpp =================
#include "runtime.h"
#include "fib.h"

bool PERFORM_VALIDATION = false;
int FINAL_RESULT = 0;

int main(int argc, char** argv) {
  int numThreads = 8;
  if (argc > 1) {
      std::string arg1 = argv[1];
      if (arg1 == "--validate") {
          PERFORM_VALIDATION = true;
          if (argc > 2) numThreads = atoi(argv[2]);
      } else {
          numThreads = atoi(argv[1]);
          if (argc > 2 && std::string(argv[2]) == "--validate") {
              PERFORM_VALIDATION = true;
          }
      }
  }

  Runtime<FibArgs, Worker<FibArgs, FuncType>> rt(numThreads);
  rt.run();

  if (PERFORM_VALIDATION) {
      int expected = serial_fib(FIB_INPUT);
      if (FINAL_RESULT == expected) {
          std::cout << "Validation Passed\n";
      } else {
          std::cout << "Validation Failed: Expected " << expected << ", got " << FINAL_RESULT << "\n";
      }
  }

  return 0;
}
