#ifndef _NUMA_CPU_BANK_H
	#define _NUMA_CPU_BANK_H

	#include <scaling.h>
	#include <__kclasses/bitmap.h>
	#include <kernel/common/task.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

class numaCpuBankC
{
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

public:
	// A bitmap of all the CPUs on the bank. Initialize with initialize().
	bitmapC		cpus;
	ubit32		capacity;
	ubit32		load;

private:
	// Count of the number of tasks on this bank for load balancing.
	sharedResourceGroupC<waitLockC, uarch_t>	nTasks;
};


/**	Inline methods:
 ******************************************************************************/

#if __SCALING__ < SCALING_SMP
inline error_t numaCpuBankC::schedule(taskC*task)
{
	return cpuTrib.getStream(0)->scheduler.schedule(task);
}
#endif

#endif

