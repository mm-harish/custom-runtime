// ================= worker.h =================
#ifndef Worker_H
#define Worker_H

#include <atomic>
#include <condition_variable>
#include <deque>
#include <emmintrin.h>
#include <functional>
#include <iostream>
#include <llvm/ADT/SmallVector.h>
#include <thread>
#include <vector>
#include <semaphore>
#include <queue>

#include "constants.h"
#include "spinlock.h"
#include "task.h"
#include "taskpool.h"
#include "readyqueue.h"

std::atomic<bool> exited[8];

/**
 * @brief Represents a worker thread in the runtime.
 * 
 * Each worker has its own task pool and ready queue. It executes tasks from its local queue
 * and steals tasks from other workers when idle.
 * 
 * @tparam Ty The type of arguments for tasks.
 * @tparam FuncTy The type of the function identifier.
 */
template <typename Ty, typename FuncTy>
struct alignas(64) Worker {
    int workerId; ///< Unique ID of the worker.

    // Type aliases for convenience and backward compatibility
    using Task = ::Task<FuncTy>;
    using TaskPool = ::TaskPool<FuncTy>;
    using ReadyQueue = ::ReadyQueue<FuncTy>;

    TaskPool pool;           ///< Pool for allocating tasks.
    ReadyQueue readyQueue;   ///< Queue of ready tasks.
    std::thread thread;      ///< The system thread running this worker.
    SpinLock waitQueueMutex; ///< Lock for accessing the steal queue.

    llvm::SmallVector<Worker<Ty, FuncTy> *, 8> workers; ///< List of all workers (for stealing).

    /**
     * @brief Constructs a new Worker.
     * @param workerId The unique ID for this worker.
     */
    Worker<Ty, FuncTy>(int workerId) {
        this->workerId = workerId;
        exited[workerId].store(false, std::memory_order_relaxed);
    }

    /**
     * @brief Sets the list of all workers.
     * @param workers Vector of worker pointers.
     */
    void inline setWorkers(llvm::SmallVector<Worker<Ty, FuncTy> *, 8> &workers) {
        this->workers = workers;
    }

    /**
     * @brief Joins the worker thread.
     */
    void inline join() {
        if (thread.joinable())
            thread.join();
    }

    void __attribute__((preserve_none)) spawn(int, Task *, int, int, bool);
    void __attribute__((preserve_none)) sync(int, Task *, int, int, int);

    /**
     * @brief Creates a new synchronization frame (task) and a child task.
     * 
     * @param val The slot index.
     * @param addr The return address task.
     * @param left The argument for the child task.
     * @return The new synchronization task.
     */
    inline Task* createNewSyncFrameCustom(int val, Task* addr, int left) {
        Task* newTask = pool.getFrame();
        Task* leftTask = pool.getFrame();

        __builtin_prefetch(&newTask->remainingInputs, 1, 3);
        newTask->funcType = FuncTy::SYNC;
        newTask->slot = val;
        newTask->address = addr;
        newTask->addressOwner = workerId;
        leftTask->funcType = FuncTy::SPAWN;
        leftTask->args[0] = left;
        leftTask->address = newTask;
        leftTask->slot = 1;
        leftTask->addressOwner = workerId;

        if (readyQueue.isLocalQueueFull()) {
            newTask->remainingInputs.store(2, std::memory_order_relaxed);
            newTask->expectLastProducer = false;
            leftTask->lastProducer = false;
            waitQueueMutex.lock();
            readyQueue.steal_push_back(leftTask);
            waitQueueMutex.unlock();
        } else {
            if (left < 1) {
                newTask->expectLastProducer = true;
                leftTask->lastProducer = true;
            } else {
                newTask->remainingInputs.store(2, std::memory_order_relaxed);
                newTask->expectLastProducer = false;
                leftTask->lastProducer = false;
            }
            readyQueue.local_push_back(leftTask);
        }
        return newTask;
    }

    /**
     * @brief Creates a new spawn frame and pushes it to the queue.
     * @param left The argument for the task.
     * @param slot The slot index.
     */
    __attribute__((always_inline))
    void inline createNewSpawnFrameAndWriteArgs(int left, int slot) {
        Task* newTask = pool.getFrame();
        newTask->funcType = FuncTy::SPAWN;
        newTask->args[0] = left;
        newTask->address = nullptr;
        newTask->slot = slot;
        newTask->addressOwner = workerId;
        if (readyQueue.isLocalQueueFull()) {
            waitQueueMutex.lock();
            readyQueue.steal_push_back(newTask);
            waitQueueMutex.unlock();
        } else {
            readyQueue.local_push_back(newTask);
        }
    }

    /**
     * @brief Creates a new spawn frame and launches it immediately.
     * @param left The argument for the task.
     * @param address The return address task.
     * @param slot The slot index.
     */
    __attribute__((always_inline))
    void inline createNewSpawnFrameAndWriteArgsAndLaunch(int left, Task* address, int slot) {
        spawn(left, address, slot, address->addressOwner, false);
    }

    /**
     * @brief Writes a value to a task's argument slot.
     * 
     * Handles decrementing the input count and scheduling the task if ready.
     * 
     * @param task The task to write to.
     * @param slot The slot index.
     * @param val The value to write.
     * @param enqueueLocally Whether to enqueue locally if ready.
     * @param lastProducer Whether this is the last producer (optimization).
     */
    __attribute__((always_inline))
    void inline writeDataToFrameImpl(Task *task, int slot, int val, bool enqueueLocally, bool lastProducer) {
        task->setValue(slot, val);
        if (task->expectLastProducer) {
            if (!lastProducer) {
                return;
            } else {
                _mm_prefetch(&readyQueue, _MM_HINT_T0);
                if (enqueueLocally && !readyQueue.isLocalQueueFull()) {
                    readyQueue.local_push_back(task);
                    return;
                }
                waitQueueMutex.lock();
                readyQueue.steal_push_back(task);
                waitQueueMutex.unlock();
                return;
            }
        } else if (task->remainingInputs.fetch_sub(1, std::memory_order_relaxed) == 1) {
            _mm_prefetch(&readyQueue, _MM_HINT_T0);
            if (enqueueLocally && !readyQueue.isLocalQueueFull()) {
                readyQueue.local_push_back(task);
                return;
            }
            waitQueueMutex.lock();
            readyQueue.steal_push_back(task);
            waitQueueMutex.unlock();
            return;
        }
    }

    /**
     * @brief Writes a return address to a task.
     * 
     * @param task The task to write to.
     * @param slot The slot index.
     * @param val The address task.
     * @param enqueueLocally Whether to enqueue locally if ready.
     */
    void inline writeAddressToFrameImpl(Task *task, int slot, Task *val, bool enqueueLocally) {
        task->address = val;
        if (task->remainingInputs.fetch_sub(1, std::memory_order_relaxed) == 1) {
            _mm_prefetch(&readyQueue, _MM_HINT_T0);
            if (enqueueLocally && !readyQueue.isLocalQueueFull()) {
                readyQueue.local_push_back(task);
                return;
            }
            waitQueueMutex.lock();
            readyQueue.steal_push_back(task);
            waitQueueMutex.unlock();
            return;
        }
    }

    /**
     * @brief Tries to execute a task from the local queue.
     * @return The executed task, or nullptr if none found (and steal failed).
     */
    inline Task* executeLocalTask() {
        if (!readyQueue.isLocalQueueEmpty()) {
            auto task = readyQueue.local_pop_back();
            return task;
        }
        waitQueueMutex.lock();
        Task *t = readyQueue.steal_pop_back();
        waitQueueMutex.unlock();
        return t;
    }

    /**
     * @brief Steals a task from another worker.
     * @param id The ID of the victim worker.
     * @return The stolen task, or nullptr if failed.
     */
    long stealAttempts{0};   ///< Number of steal attempts.
    long stealSuccesses{0};  ///< Number of successful steals.
    long tasksExecuted{0};   ///< Number of tasks executed.

    /**
     * @brief Prints the worker's performance statistics.
     */
    void printStats() {
        std::cout << "Worker " << workerId << ": "
                  << "Tasks Executed = " << tasksExecuted << ", "
                  << "Steal Attempts = " << stealAttempts << ", "
                  << "Steal Successes = " << stealSuccesses << "\n";
    }

    /**
     * @brief Steals a task from another worker.
     * @param id The ID of the victim worker.
     * @return The stolen task, or nullptr if failed.
     */
    inline Task* stealRemoteTask(int id) {
        stealAttempts++;
        workers[id]->waitQueueMutex.lock();
        Task* frameId = workers[id]->readyQueue.steal_pop_front();
        workers[id]->waitQueueMutex.unlock();
        if (frameId) stealSuccesses++;
        return frameId;
    }

    /**
     * @brief The main worker loop.
     * 
     * Continuously executes tasks until all workers have exited.
     */
    __attribute__((hot, flatten)) void workerLoop() {
        while (true) {
            // try to pop from my readyQueue first
            Task* t = executeLocalTask();
            if (t) {
                FuncTy fn = t->funcType;
                int left = t->args[0];
                int right = t->args[1];
                Task* address = t->address;
                int slot = t->slot;
                int addressOwner = t->addressOwner;
                bool lastProducer = t->lastProducer;
                pool.free(t);
                tasksExecuted++;
                if (fn == FuncTy::SPAWN) {
                    spawn(left, address, slot, addressOwner, lastProducer);
                } else {
                    sync(left, address, right, slot, addressOwner);
                }
                continue;
            } else {
                for (int i = 0; i < workers.size(); i++) {
                    if (i == workerId)
                        continue;
                    _mm_prefetch(&workers[i]->waitQueueMutex, _MM_HINT_T0);
                    _mm_prefetch(&workers[i]->readyQueue, _MM_HINT_T0);
                    t = stealRemoteTask(i);
                    if (t) {
                        break;
                    }
                }
            }
            bool end = false;
            if (t) {
                FuncTy fn = t->funcType;
                int left = t->args[0];
                int right = t->args[1];
                Task* address = t->address;
                int slot = t->slot;
                bool lastProducer = t->lastProducer;
                int addressOwner = t->addressOwner;
                pool.free(t);
                tasksExecuted++;
                if (fn == FuncTy::SPAWN) {
                    spawn(left, address, slot, addressOwner, lastProducer);
                } else {
                    sync(left, address, right, slot, addressOwner);
                }
                continue;
            } else {
                for (int i = 0; i < workers.size(); i++) {
                    end = exited[i].load(std::memory_order_relaxed);
                    if (end)
                        break;
                }
                if (!end)
                    std::this_thread::yield();
            }
            if (end) {
                break;
            }
        }
    }

    /**
     * @brief Starts the worker thread.
     */
    void start() {
        thread = std::thread(&Worker::workerLoop, this);
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        int num_cpus = std::thread::hardware_concurrency();
        CPU_SET(workerId % num_cpus, &cpuset);

        int r = pthread_setaffinity_np(thread.native_handle(), sizeof(cpu_set_t),
                                       &cpuset);
        if (r != 0) {
            perror("pthread_setaffinity_np");
        }
    }
};

#endif
