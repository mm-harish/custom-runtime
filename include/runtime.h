// ================= runtime.h =================
#ifndef RUNTIME_H
#define RUNTIME_H
#include "wsqueue.h"

/**
 * @brief The Runtime class manages the worker threads and task execution.
 * 
 * @tparam A The type of arguments for the tasks.
 * @tparam B The type of worker.
 */
template <typename A, typename B>
class Runtime {
public:
    llvm::SmallVector<B*, 8> workers; ///< List of worker threads.

    /**
     * @brief Construct a new Runtime object.
     * 
     * @param numThreads Number of worker threads to create.
     */
    Runtime<A>(int numThreads);

    /**
     * @brief Start the runtime and execute tasks.
     */
    void run();

    /**
     * @brief Initialize the runtime environment.
     */
    void init();
};

// Generic template implementations
template <typename A, typename B>
Runtime<A, B>::Runtime(int numThreads) {
    for(int i = 0; i < numThreads; i++) {
        B* worker = new B(i);
        workers.push_back(worker);
    }
    
    for(int i = 0; i < numThreads; i++) {
        workers[i]->setWorkers(workers);
    }
}

template <typename A, typename B>
void Runtime<A, B>::run() {
    init();
    for (auto w : workers) 
        w->start();
    for (auto w : workers) 
        w->join();
    for (auto w : workers)
        w->printStats();
}

#endif
