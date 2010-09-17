#ifndef _NUMA_CPU_BANK_H
	#define _NUMA_CPU_BANK_H

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

	// Cpu task pull request functions.
	// taskS *taskPullRequest(void);
	// taskS *taskRejectRequest(taskS *prevTask);

private:
	// A bitmap of all the CPUs on the bank. Initialize with initialize().
	bitmapC		cpus;

	// Count of the number of tasks on this bank for load balancing.
	sharedResourceGroupC<waitLockC, uarch_t>	nTasks;
};

#endif

