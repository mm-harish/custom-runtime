#ifndef TASKPOOL_H
#define TASKPOOL_H

#include "constants.h"
#include "task.h"
#include <iostream>
#include <vector>

/**
 * @brief Manages a pool of tasks to reduce allocation overhead.
 *
 * @tparam FuncTy The type of the function identifier.
 */
template <typename FuncTy> struct alignas(64) TaskPool {
  using TaskType = Task<FuncTy>;
  TaskType *front[numframes]; ///< Current block of free tasks.
  int frontIndex{numframes};  ///< Index in the current block.
  TaskType *freePoolFront[n]; ///< Global free pool of tasks.
  int freePoolIndex{0};       ///< Index in the global free pool.
  std::vector<TaskType *>
      allocatedPointers; ///< Keeps track of allocated blocks for cleanup.

  /**
   * @brief Allocates a new block of frames.
   * @return A pointer to the first task in the new block.
   */
  __attribute__((cold)) inline TaskType *allocateFrame() {
    TaskType *tasks = new TaskType[numframes];
    allocatedPointers.push_back(tasks);
    for (int i = 0; i < numframes; i++)
      front[i] = &tasks[i];
    TaskType *t = &tasks[0];
    frontIndex = 1;
    return t;
  }

  /**
   * @brief Allocates a new block and returns two frames.
   * @return A pair of pointers to the first two tasks.
   */
  __attribute__((cold)) inline std::pair<TaskType *, TaskType *>
  allocateTwoFrames() {
    TaskType *tasks = new TaskType[numframes];
    allocatedPointers.push_back(tasks);
    for (int i = 0; i < numframes; i++)
      front[i] = &tasks[i];
    frontIndex = 2;
    return std::make_pair(&tasks[0], &tasks[1]);
  }

  __attribute__((hot)) inline bool hasTwoFrames() {
    return frontIndex < numframes - 1 || freePoolIndex >= 2;
  }

  /**
   * @brief Gets two frames from the pool.
   * @return A pair of task pointers.
   */
  __attribute__((hot)) inline std::pair<TaskType *, TaskType *> getTwoFrames() {
    if (frontIndex >= numframes - 1 && freePoolIndex < 2) {
      return allocateTwoFrames();
    } else if (frontIndex < numframes - 1) {
      auto t = std::make_pair(front[frontIndex], front[frontIndex + 1]);
      frontIndex = frontIndex + 2;
      return t;
    } else if (freePoolIndex >= 2) {
      auto ret = std::make_pair(freePoolFront[freePoolIndex - 1],
                                freePoolFront[freePoolIndex - 2]);
      freePoolIndex -= 2;
      return ret;
    }
    return std::make_pair(nullptr, nullptr);
  }

  /**
   * @brief Gets a single frame from the pool.
   * @return A pointer to a task.
   */
  __attribute__((hot)) inline TaskType *getFrame() {
    if (frontIndex == numframes && freePoolIndex == 0)
      return allocateFrame();
    if (frontIndex < numframes) {
      return front[frontIndex++];
    }
    if (freePoolIndex > 0) {
      return freePoolFront[--freePoolIndex];
    }
    return nullptr;
  }

  /**
   * @brief Returns a task to the pool.
   * @param t The task to free.
   * @param enqueOnBack Unused parameter.
   */
  inline void free(TaskType *t, bool enqueOnBack = false) {
    if (freePoolIndex == n) {
      std::cout << "free pool full, cant free\n";
      exit(1);
    }
    freePoolFront[freePoolIndex++] = t;
  }

  /**
   * @brief Destructor. Cleans up all allocated blocks.
   */
  ~TaskPool() {
    for (int i = 0; i < allocatedPointers.size(); i++)
      delete[] allocatedPointers[i];
  }
};

#endif // TASKPOOL_H
