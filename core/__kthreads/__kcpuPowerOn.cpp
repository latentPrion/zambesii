
#include <kernel/common/__koptimizationHacks.h>
#include <kernel/common/processTrib/processTrib.h>
#include <__kthreads/__kcpuPowerOn.h>

/**	EXPLANATION:
 * This block holds all the necessary data that a waking CPU will need:
 *	* A stack to use; specifically its own sleep stack.
 *	* The address of its own CPU Stream within the kernel vaddrspace.
 *	* A lock to protect this variable from being modified until the CPU
 *	  has copied all of the information.
 **/
struct __kcpuPowerOnBlockS	__kcpuPowerOnBlock;

/**	EXPLANATION:
 * Under here is the __kcpuPowerOn thread's actual thread structure, as
 * a recognizible entity by the scheduler.
 *
 * This is what the waking CPU's stream will point to as its "currentTask". Its
 * members are initialized by the CPU Tributary before any CPUs are awakened in
 * cpuTribC::initialize2().
 **/
taskC	__kcpuPowerOnThread(0x1, &processTrib.__kprocess);

// Part of __koptimizationHacks.cpp.
void (*__kcpuPowerOnInit(void))()
{
	return &__kcpuPowerOnEntry;
}
