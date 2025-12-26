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

#endif
