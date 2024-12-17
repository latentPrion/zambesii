#ifndef _NUMA_CPU_BANK_H
	#define _NUMA_CPU_BANK_H

	#include <scaling.h>
	#include <__kclasses/bitmap.h>
	#include <__kclasses/sortedHeapDoubleList.h>
	#include <kernel/common/numaTypes.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>
	#include <kernel/common/timerTrib/timeTypes.h>
	#include <kernel/common/taskTrib/load.h>

// This class is only used in NUMA builds of the kernel.
#if __SCALING__ >= SCALING_CC_NUMA
class Thread;

class NumaCpuBank
{
public:
	NumaCpuBank(void);
	error_t initialize(uarch_t nCpuBits);
	// Used to set up the fake __kspace NUMA bank.
	void __kspaceInitialize(void);

public:
	// Halt or restart all logical CPUs on this bank. Stub for now.
	void cut(void) {};
	void bind(void) {};

public:
	// Chooses the best CPU on this bank to schedule the new task to.
	error_t schedule(Thread *thread);

	ubit32 getLoad(void) { return load; };
	ubit32 getCapacity(void) { return capacity; };
	void updateLoad(ubit8 action, ubit32 val);
	void updateCapacity(ubit8 action, ubit32 val);

	error_t addProximityInfo(
		numaBankId_t bankId, sTime latency, sbit32 acpiProximityFactor);

	void removeProximityInfo(numaBankId_t bankId);

public:
	// A bitmap of all the CPUs on the bank. Initialize with initialize().
	Bitmap		cpus;

	struct sMemProximityEntry
	{
		numaBankId_t	bankId;
		sbit32		acpiProximityFactor;
		sTime		latency;
	};

	/*	EXPLANATION:
	 * Sorted list of memory banks reachable from this node, ordered by
	 * latency. The implication is that banks that aren't reachable aren't
	 * listed.
	 **/
	SortedHeapDoubleList<sMemProximityEntry, sTime>
		memProximityMatrix;

	ubit32		capacity;
	ubit32		load;

private:
	// Count of the number of tasks on this bank for load balancing.
	SharedResourceGroup<WaitLock, uarch_t>	nTasks;
};

#endif /* if __SCALING__ >= SCALING_CC_NUMA */

#endif

