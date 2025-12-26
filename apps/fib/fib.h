// ================= fib.h =================
#ifndef Fib_H
#define Fib_H
#include <atomic>
#include <condition_variable>
#include <functional>
#include <wsqueue.h>
#include <iostream>
#include "runtime.h"

enum FuncType:bool{
	SPAWN = 0,
	SYNC = 1,
};

struct FibArgs{
	int left;
	int right;
	int slot;
	Worker<FibArgs, FuncType>::Task* address;
};

extern bool PERFORM_VALIDATION;
extern int FINAL_RESULT;

/// @cond DOXYGEN_SKIP
template<> 
void __attribute__((hot)) __attribute__((preserve_none)) Worker<FibArgs, FuncType>::spawn(int left, Worker<FibArgs, FuncType>::Task* address, int slot, int addressOwner, bool lastProducer){
		if(left >= 2){
			//createFibChildrenAndLaunch(slot, address, left);
			auto syncTaskId = createNewSyncFrameCustom(slot, address, left - 2);
	    	        assert(syncTaskId->funcType == FuncType::SYNC);
			createNewSpawnFrameAndWriteArgsAndLaunch(left - 1, syncTaskId, 0);
		}
		else if(address){
  		    _mm_prefetch(&address->args, _MM_HINT_T0);		    		    		    		    
		    __builtin_prefetch(&address->remainingInputs, 1, 3);
		    if(addressOwner == workerId)
		    	writeDataToFrameImpl(address, slot, left, true, lastProducer);
		    else
		        workers[addressOwner]->writeDataToFrameImpl(address, slot, left, false, false);
		}
		return;
}

const int FIB_INPUT = 40;

int serial_fib(int input) {
    if (input < 2) return input;
    return serial_fib(input - 1) + serial_fib(input - 2);
}

template<> 
void __attribute__((hot)) __attribute__((preserve_none)) Worker<FibArgs, FuncType>::sync(int left, Worker<FibArgs, FuncType>::Task* address, int right, int slot, int addressOwner){
		int sum = left + right;
		if(address){
   		   if(addressOwner == workerId){
		   	writeDataToFrameImpl(address, slot, sum, true, false);
		   }else{
		   	workers[addressOwner]->writeDataToFrameImpl(address, slot, sum, false, false);
		   }
		}
   		else{
   			std::cout<<"sum:"<<sum<<"\n";
            FINAL_RESULT = sum;
   			exited[workerId].store(true, std::memory_order_relaxed);
   			std::atomic_thread_fence(std::memory_order_release);
   		}
   		return;
}
/// @endcond

template<>
void Runtime<FibArgs, Worker<FibArgs,FuncType>>::init(){
    ((Worker<FibArgs, FuncType>*)workers[0])->createNewSpawnFrameAndWriteArgs(FIB_INPUT, 0);
}


#endif
