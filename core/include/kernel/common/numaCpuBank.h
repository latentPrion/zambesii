#ifndef _NUMA_CPU_BANK_H
	#define _NUMA_CPU_BANK_H

	#include <scaling.h>
	#include <__kclasses/bitmap.h>
	#include <__kclasses/sortedPtrDoubleList.h>
	#include <kernel/common/task.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>
	#include <kernel/common/timerTrib/timeTypes.h>
	#include <kernel/common/taskTrib/load.h>

// This class is only used in NUMA builds of the kernel.
#if __SCALING__ >= SCALING_CC_NUMA
class numaCpuBankC
{
public:
	numaCpuBankC(void);
	error_t initialize(uarch_t nCpuBits);
	// Used to set up the fake __kspace NUMA bank.
	void __kspaceInitialize(void);

public:
	// Halt or restart all logical CPUs on this bank. Stub for now.
	void cut(void) {};
	void bind(void) {};

public:
	// Chooses the best CPU on this bank to schedule the new task to.
	error_t schedule(taskC*task);

	ubit32 getLoad(void) { return load; };
	ubit32 getCapacity(void) { return capacity; };
	void updateLoad(ubit8 action, ubit32 val);
	void updateCapacity(ubit8 action, ubit32 val);

	error_t addProximityInfo(
		numaBankId_t bankId, timeS latency, sbit32 acpiProximityFactor);

	void removeProximityInfo(numaBankId_t bankId);

public:
	// A bitmap of all the CPUs on the bank. Initialize with initialize().
	bitmapC		cpus;
	struct memProximityEntryS
	{
		numaBankId_t	bankId;
		sbit32		acpiProximityFactor;
		timeS		latency;
	};

	sortedPointerDoubleListC<memProximityEntryS, timeS>
		memProximityMatrix;

	ubit32		capacity;
	ubit32		load;

private:
	// Count of the number of tasks on this bank for load balancing.
	sharedResourceGroupC<waitLockC, uarch_t>	nTasks;
};

#endif /* if __SCALING__ >= SCALING_CC_NUMA */

#endif

