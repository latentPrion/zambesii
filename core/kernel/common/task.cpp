
#include <scaling.h>
#include <chipset/memory.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/task.h>
#include <kernel/common/process.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


error_t taskC::initialize(void)
{
	error_t		ret;

#ifdef CONFIG_PER_TASK_TLB_CONTEXT
	ret = tlbContext.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };
#endif

#if __SCALING__ >= SCALING_SMP
	ret = cpuAffinity.initialize(cpuTrib.availableCpus.getNBits());
	if (ret != ERROR_SUCCESS) { return ret; };
#endif

	ret = registeredEvents.initialize(32);
	if (ret != ERROR_SUCCESS) { return ret; };

	ret = timerStreamEvents.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	return ERROR_SUCCESS;
}

error_t taskC::allocateStacks(void)
{
	/**	NOTES:
	 * Ideally there should be a special allocation function for stack
	 * allocation. For now, just take whatever comes from Memory Stream.
	 **/
	// No fakemapping for kernel stacks.
	stack0 = processTrib.__kprocess.memoryStream.memAlloc(
		CHIPSET_MEMORY___KSTACK_NPAGES, MEMALLOC_NO_FAKEMAP);

	if (stack0 == __KNULL)
	{
		__kprintf(ERROR TASK"0x%x: kernel stack alloc failed.\n", id);
		return ERROR_MEMORY_NOMEM;
	};

	context = (taskContextC *)((uarch_t)stack0
		+ CHIPSET_MEMORY___KSTACK_NPAGES * PAGING_BASE_SIZE);

	context = (taskContextC *)((uarch_t)context - sizeof(taskContextC));
	new (context) taskContextC(parent->execDomain);
	context->initialize();

	// Don't allocate a user stack for threads of kernel space processes.
	if (parent->execDomain != PROCESS_EXECDOMAIN_USER) {
		return ERROR_SUCCESS;
	};

	stack1 = parent->memoryStream.memAlloc(CHIPSET_MEMORY_USERSTACK_NPAGES);
	if (stack1 == __KNULL)
	{
		processTrib.__kprocess.memoryStream.memFree(stack0);
		__kprintf(ERROR TASK"0x%x: failed to alloc user stack.\n", id);
		return ERROR_MEMORY_NOMEM;
	};

	return ERROR_SUCCESS;
}

static inline error_t resizeAndMergeBitmaps(bitmapC *dest, bitmapC *src)
{
	error_t		ret;

	ret = dest->resizeTo(src->getNBits());
	if (ret != ERROR_SUCCESS) { return ret; };

	dest->merge(src);
	return ERROR_SUCCESS;
}

#if __SCALING__ >= SCALING_SMP
error_t taskC::inheritAffinity(bitmapC *cpuAffinity, uarch_t flags)
{
	error_t		ret;

	/**	EXPLANATION:
	 * Threads STINHERIT CPU Affinity by default. Overrides are:
	 * PINHERIT:
	 *	Indicated by SPAWNTHREAD_FLAGS_AFFINITY_PINHERIT.
	 * SET:
	 *	Implied if cpuAffinity != __KNULL.
	 **/
	if (cpuAffinity == __KNULL)
	{
		// Default: STINHERIT.
		bitmapC		*sourceBitmap;

		if (__KFLAG_TEST(flags, SPAWNTHREAD_FLAGS_AFFINITY_PINHERIT)) {
			sourceBitmap = &parent->cpuAffinity;
		}
		else
		{
			sourceBitmap = &cpuTrib.getCurrentCpuStream()
				->taskStream.currentTask->cpuAffinity;
		};

		ret = resizeAndMergeBitmaps(&this->cpuAffinity, sourceBitmap);
		if (ret != ERROR_SUCCESS) { return ret; };
	}
	else
	{
		ret = resizeAndMergeBitmaps(&this->cpuAffinity, cpuAffinity);
		if (ret != ERROR_SUCCESS) { return ret; };
	};

	return ERROR_SUCCESS;
}
#endif

void taskC::inheritSchedPolicy(schedPolicyE schedPolicy, uarch_t /*flags*/)
{
	/**	EXPLANATION:
	 * Sched policy is STINHERITed by default. Overrides are:
	 * SET:
	 *	Taken if schedPolicy != 0.
	 **/
	if ((int)schedPolicy != 0)
	{
		// SET.
		this->schedPolicy = schedPolicy;
	}
	else
	{
		// STINHERIT.
		this->schedPolicy = cpuTrib.getCurrentCpuStream()->taskStream
			.currentTask->schedPolicy;
	};
}

void taskC::inheritSchedPrio(prio_t prio, uarch_t flags)
{
	/**	EXPLANATION:
	 * Schedprio defaults to PRIOCLASS_DEFAULT. Overrides are:
	 * STINHERIT:
	 *	if SPAWNTHREAD_FLAGS_SCHEDPRIO_STINHERIT.
	 * PRIOCLASS:
	 *	if SPAWNTHREAD_FLAGS_SCHEDPRIO_PRIOCLASS.
	 * SET:
	 *	if SPAWNTHREAD_FLAGS_SCHEDPRIO_SET.
	 **/
	if (__KFLAG_TEST(flags, SPAWNTHREAD_FLAGS_SCHEDPRIO_SET))
	{
		__KFLAG_SET(this->flags, TASK_FLAGS_CUSTPRIO);
		internalPrio.prio = prio;
		schedPrio = &internalPrio;
		return;
	};

	if (__KFLAG_TEST(flags, SPAWNTHREAD_FLAGS_SCHEDPRIO_STINHERIT))
	{
		taskC		*spawningThread;

		spawningThread = cpuTrib.getCurrentCpuStream()->taskStream
			.currentTask;

		if (__KFLAG_TEST(spawningThread->flags, TASK_FLAGS_CUSTPRIO))
		{
			__KFLAG_SET(this->flags, TASK_FLAGS_CUSTPRIO);
			internalPrio.prio = spawningThread->internalPrio.prio;
			schedPrio = &internalPrio;
		}
		else {
			schedPrio = spawningThread->schedPrio;
		};

		return;
	};

	if (__KFLAG_TEST(flags, SPAWNTHREAD_FLAGS_SCHEDPRIO_PRIOCLASS_SET)
		&& prio < PRIOCLASS_NCLASSES)
	{
		schedPrio = &prioClasses[prio];
	} else {
		schedPrio = &prioClasses[PRIOCLASS_DEFAULT];
	};
}

error_t taskC::cloneStateIntoChild(taskC *child)
{
	child->flags = flags;

	child->internalPrio = internalPrio;
	if (schedPrio != &internalPrio) {
		child->schedPrio = schedPrio;
	};

	child->schedPolicy = schedPolicy;
	child->schedOptions = schedOptions;
	// 'schedFlags' and 'schedState' are not inherited.

	// Copy affinity.
	child->defaultMemoryBank.rsrc = NUMABANKID_INVALID;
	return resizeAndMergeBitmaps(&child->cpuAffinity, &cpuAffinity);
}

