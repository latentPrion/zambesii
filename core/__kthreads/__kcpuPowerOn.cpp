
#include <kernel/common/waitLock.h>
#include <kernel/common/__koptimizationHacks.h>
#include <kernel/common/processTrib/processTrib.h>
#include <__kthreads/__kcpuPowerOn.h>

/**	EXPLANATION:
 * Waking CPUs need to know the address of their sleep-stack within the kernel
 * vaddrspace in order to boot in parallel without centralized locking.
 *
 * We achieve this through the use of a global, resizeable array of pointers to
 * sleepstacks. When a CPU enters protected mode, it first looks up the address
 * that the kernel has mapped the local APICs to. Then it reads its CPU ID from
 * its local APIC, and uses that ID to index into the array of sleepstacks to
 * obtain the address of the stack it should use to enter the kernel.
 *
 * On this stack, pushed by the kernel before the CPU was powered on, is the
 * address of the CPU's CPU Stream.
 **/
waitLockC	__kcpuPowerOnSleepStacksLock;
uarch_t		__kcpuPowerOnSleepStacksLength=0;
void		**__kcpuPowerOnSleepStacks=0;
ubit8		*__kcpuPowerOnLapicVaddr=0;

/**	EXPLANATION:
 * Under here is the __kcpuPowerOn thread's actual thread structure, as
 * a recognizible entity by the scheduler.
 *
 * This is what the waking CPU's stream will point to as its "currentTask". Its
 * members are initialized by the CPU Tributary before any CPUs are awakened in
 * cpuTribC::initialize2().
 **/
perCpuThreadC	__kcpuPowerOnThread(processTrib.__kgetStream());

// Part of __koptimizationHacks.cpp.
void (*__kcpuPowerOnInit(void))()
{
	return &__kcpuPowerOnEntry;
}
