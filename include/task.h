#ifndef TASK_H
#define TASK_H

#include <atomic>

/**
 * @brief Represents a unit of work in the runtime.
 *
 * @tparam FuncTy The type of the function identifier (e.g., enum).
 */
template <typename FuncTy> struct alignas(64) Task {
  alignas(64) std::atomic<int32_t> remainingInputs{
      0};                  ///< Number of inputs this task is waiting for.
  int args[2];             ///< Arguments for the task function.
  FuncTy funcType;         ///< The type of function to execute.
  Task *__restrict__ address{nullptr}; ///< Return address or parent task.
  int slot;         ///< Slot index in the parent task to write the result.
  int addressOwner; ///< Worker ID that owns the address task.
  bool lastProducer{
      false}; ///< Flag indicating if this is the last producer for a join.
  bool expectLastProducer{false}; ///< Flag indicating if the task expects a
                                  ///< last producer optimization.

  /**
   * @brief Sets an argument value.
   *
   * @param index The index of the argument.
   * @param val The value to set.
   */
  __attribute__((preserve_none)) void inline setValue(int index, int val) {
    args[index] = val;
  }

  /**
   * @brief Sets the return address task.
   *
   * @param index Unused index (kept for signature compatibility?).
   * @param val The task to set as the address.
   */
  __attribute__((preserve_none)) void inline setAddress(int index, Task *val) {
    address = val;
  }
};

#endif // TASK_H
