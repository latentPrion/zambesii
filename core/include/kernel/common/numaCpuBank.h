#ifndef _NUMA_CPU_BANK_H
	#define _NUMA_CPU_BANK_H

	#include <__kclasses/bitmap.h>
	#include <kernel/common/task.h>

/**	EXPLANATION:
 * The NUMA CPU Bank's main purpose is to cause a particular bank of CPU's
 * scheduling activities to be halted. The design of Zambezii's scheduler is
 * such that the main scheduler simply holds a list of tasks in a list of
 * 'flowing' (runnable) processes.
 *
 * A CPU will 'pull' a task from its NUMA CPU bank. The NUMA CPU banks will pull
 * tasks from the main scheduler. To get a new task, a CPU calls
 *	taskS *numaTribC::getStream(myBankId)taskPullRequest();
 *
 * The NUMA trib will return a pointer to a task on that CPU's bank. The CPU
 * will then test to see what kind of affinity the task has, whether SMP or
 * NUMA. If the affinity is NUMA, then the CPU will simply go on with execution.
 *
 * If the affinity is SMP, then the CPU must test to see which CPU the task is
 * bound to. If the SMP affinity is not tied to the current CPU, the CPU must
 * reject the task, by calling:
 *	taskS *numaTribC::getStream(myBankId)::taskRejectRequest();
 *
 * Which will return the pulled task to the scheduler. Simultaneously, the
 * scheduler will return another task, which the CPU will check yet again, and
 * keep pulling until it gets a task which it can execute.
 **/

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
};

#endif

