#ifndef READYQUEUE_H
#define READYQUEUE_H

#include "constants.h"
#include "task.h"
#include <iostream>

/**
 * @brief A work-stealing queue implementation.
 * 
 * Maintains a local LIFO queue for the owner worker and a public FIFO queue for stealing.
 * 
 * @tparam FuncTy The type of the function identifier.
 */
template <typename FuncTy>
struct alignas(64) ReadyQueue {
    using TaskType = Task<FuncTy>;
    TaskType* readyLocalQueue[m]; ///< Local LIFO queue.
    int localQueueBack{0};        ///< Back index of the local queue.

    TaskType* readyStealQueue[n]; ///< Public FIFO steal queue.
    int front{0};                 ///< Front index of the steal queue.
    int back{0};                  ///< Back index of the steal queue.

    /**
     * @brief Checks if the local queue is full.
     * @return True if full, false otherwise.
     */
    __attribute__((hot))
    bool inline isLocalQueueFull() {
        return localQueueBack == m;
    }

    /**
     * @brief Pushes a task to the local queue.
     * @param t The task to push.
     */
    __attribute__((hot))
    void local_push_back(TaskType* t) {
        readyLocalQueue[localQueueBack++] = t;
    }

    /**
     * @brief Checks if the local queue is empty.
     * @return True if empty, false otherwise.
     */
    __attribute__((hot))
    bool inline isLocalQueueEmpty() {
        return !localQueueBack;
    }

    /**
     * @brief Pops a task from the local queue.
     * @return The popped task, or nullptr if empty.
     */
    __attribute__((hot))
    TaskType* local_pop_back() {
        if (localQueueBack == 0)
            return nullptr;
        auto t = readyLocalQueue[--localQueueBack];
        return t;
    }

    /**
     * @brief Pushes a task to the steal queue (back).
     * @param t The task to push.
     */
    __attribute__((hot))
    void steal_push_back(TaskType* t) {
        if (back - front == n) {
            std::cout << "queue full\n";
            exit(0);
        }
        readyStealQueue[back++ & (n - 1)] = t;
    }

    /**
     * @brief Pops a task from the steal queue (front) - used by thieves.
     * @return The popped task, or nullptr if empty.
     */
    __attribute__((cold))
    TaskType* steal_pop_front() {
        if (back == front) {
            return nullptr;
        }
        return readyStealQueue[front++ & (n - 1)];
    }

    /**
     * @brief Pops a task from the steal queue (back) - used by owner when local is empty.
     * @return The popped task, or nullptr if empty.
     */
    __attribute__((cold))
    TaskType* steal_pop_back() {
        if (back == front) {
            return nullptr;
        }
        return readyStealQueue[--back & (n - 1)];
    }
};

#endif // READYQUEUE_H
