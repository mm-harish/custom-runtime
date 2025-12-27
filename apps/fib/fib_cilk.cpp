// ================= fib_cilk.cpp =================
// OpenCilk implementation of Fibonacci for comparison with custom runtime
#include <cilk/cilk.h>
#include <cilk/cilk_api.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <chrono>

const int FIB_INPUT = 40;

/**
 * @brief Parallel Fibonacci computation using OpenCilk
 * 
 * @param n The Fibonacci number to compute
 * @return The nth Fibonacci number
 */
long fib_cilk(int n) {
    if (n < 2) {
        return n;
    }
    
    long x, y;
    x = cilk_spawn fib_cilk(n - 1);
    y = fib_cilk(n - 2);
    cilk_sync;
    
    return x + y;
}

/**
 * @brief Serial Fibonacci computation for validation
 * 
 * @param n The Fibonacci number to compute
 * @return The nth Fibonacci number
 */
long serial_fib(int n) {
    if (n < 2) {
        return n;
    }
    return serial_fib(n - 1) + serial_fib(n - 2);
}

int main(int argc, char **argv) {
    int num_threads = 8;
    bool perform_validation = false;
    
    // Parse command-line arguments (same format as custom runtime version)
    if (argc > 1) {
        std::string arg1 = argv[1];
        if (arg1 == "--validate") {
            perform_validation = true;
            if (argc > 2) {
                num_threads = atoi(argv[2]);
            }
        } else {
            num_threads = atoi(argv[1]);
            if (argc > 2 && std::string(argv[2]) == "--validate") {
                perform_validation = true;
            }
        }
    }
    
    // Set the number of workers for Cilk runtime
    // The number of workers is controlled by the CILK_NWORKERS environment variable.
    std::string workers_str = std::to_string(num_threads);
    setenv("CILK_NWORKERS", workers_str.c_str(), 1);

    
    // Run the parallel Fibonacci computation
    auto start = std::chrono::high_resolution_clock::now();
    long result = fib_cilk(FIB_INPUT);
    auto end = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "sum:" << result << "\n";
    std::cout << "Computation Time: " << elapsed.count() << " seconds\n";
    
    // Validation (if requested)
    if (perform_validation) {
        long expected = serial_fib(FIB_INPUT);
        if (result == expected) {
            std::cout << "Validation Passed\n";
        } else {
            std::cout << "Validation Failed: Expected " << expected 
                      << ", got " << result << "\n";
        }
    }
    
    return 0;
}
