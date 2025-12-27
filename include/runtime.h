// ================= runtime.h =================
#ifndef RUNTIME_H
#define RUNTIME_H
#include "wsqueue.h"

/**
 * @brief The Runtime class manages the worker threads and task execution.
 *
 * @tparam B The type of worker.
 */
template <typename B> class Runtime {
public:
  llvm::SmallVector<B *, 8> workers; ///< List of worker threads.

  /**
   * @brief Construct a new Runtime object.
   *
   * @param numThreads Number of worker threads to create.
   */
  Runtime(int numThreads);

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
template <typename B> Runtime<B>::Runtime(int numThreads) {
  for (int i = 0; i < numThreads; i++) {
    B *worker = new B(i);
    workers.push_back(worker);
  }

  for (int i = 0; i < numThreads; i++) {
    workers[i]->setWorkers(workers);
  }
}

template <typename B> void Runtime<B>::run() {
  for (auto w : workers)
    w->start();

  init();
  for (auto w : workers)
    w->join();

#ifndef PERFORM_VALIDATION
  for (auto w : workers)
    w->printStats();
#endif
}

#endif
