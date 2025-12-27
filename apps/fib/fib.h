// ================= fib.h =================
#ifndef Fib_H
#define Fib_H
#include "runtime.h"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <wsqueue.h>

enum FuncType : bool { SPAWN = 0, SYNC = 1 };

extern int FINAL_RESULT;

/// @cond DOXYGEN_SKIP
template <>
void __attribute__((hot)) __attribute__((preserve_none))
Worker<FuncType>::spawn(int left, Worker<FuncType>::Task *address, int slot,
                        int addressOwner, bool lastProducer) {
  if (left >= 2) {
    // createFibChildrenAndLaunch(slot, address, left);
    auto syncTaskId = createNewSyncFrameCustom(slot, address, left - 2);
    createNewSpawnFrameAndWriteArgsAndLaunch(left - 1, syncTaskId, 0);
  } else {
    _mm_prefetch(&address->args, _MM_HINT_T0);
    __builtin_prefetch(&address->remainingInputs, 1, 3);
    if (addressOwner == workerId)
      writeDataToFrameImpl(address, slot, left, true, lastProducer);
    else
      workers[addressOwner]->writeDataToFrameImpl(address, slot, left, false,
                                                  false);
  }
  return;
}

const int FIB_INPUT = 40;

int serial_fib(int input) {
  if (input < 2)
    return input;
  return serial_fib(input - 1) + serial_fib(input - 2);
}

template <>
void __attribute__((hot)) __attribute__((preserve_none))
Worker<FuncType>::sync(int left, Worker<FuncType>::Task *address, int right,
                       int slot, int addressOwner) {
  int sum = left + right;
  if (address) {
    if (addressOwner == workerId) {
      writeDataToFrameImpl(address, slot, sum, true, false);
    } else {
      workers[addressOwner]->writeDataToFrameImpl(address, slot, sum, false,
                                                  false);
    }
    return;
  } else {
    FINAL_RESULT = left + right;
    exited.store(true, std::memory_order_release);
  }
}
/// @endcond

template <> void Runtime<Worker<FuncType>>::init() {
  ((Worker<FuncType> *)workers[0])
      ->createNewSpawnFrameAndWriteArgs(FIB_INPUT, 0);
}

#endif
