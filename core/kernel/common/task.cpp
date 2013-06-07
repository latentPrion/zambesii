
#include <scaling.h>
#include <chipset/memory.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/task.h>
#include <kernel/common/process.h>
#include <kernel/common/panic.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


processId_t taskC::getFullId(void)
{
	return (parent->id << PROCID_PROCESS_SHIFT) | id;
}

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

	// The event queues use object caching to speed up alloc/free of nodes.
	ret = timerStreamEvents.initialize(
		PTRDBLLIST_INITIALIZE_FLAGS_USE_OBJECT_CACHE);

	if (ret != ERROR_SUCCESS) { return ret; };

	return ERROR_SUCCESS;
}

error_t taskC::allocateStacks(void)
{
	/**	NOTES:
	 * There are 2 separate cases for consideration here:
	 *	1. Kernel threads: worker threads for the kernel itself, which
	 *	   are part of the kernel's process. These threads do not
	 *	   allocate a userspace stack.
	 *	2. Non-kernel-process-threads: Threads for processes other than
	 *	   the kernel process. Usually processes executed in the
	 *	   userspace domain, but occasionally processes executed in the
	 *	   kernel domain. These allocate both kernelspace and userspace
	 *	   stacks.
	 **/
	// No fakemapping for kernel stacks.
	stack0 = processTrib.__kgetStream()->memoryStream.memAlloc(
		CHIPSET_MEMORY___KSTACK_NPAGES, MEMALLOC_NO_FAKEMAP);

	if (stack0 == __KNULL)
	{
		__kprintf(ERROR TASK"0x%x: kernel stack alloc failed.\n", id);
		return ERROR_MEMORY_NOMEM;
	};

	// Don't allocate a user stack for kernel threads.
	if (parent->id == __KPROCESSID) { return ERROR_SUCCESS; };

	// Allocate the stack from the parent process' memory stream.
	stack1 = parent->memoryStream.memAlloc(CHIPSET_MEMORY_USERSTACK_NPAGES);
	if (stack1 == __KNULL)
	{
		processTrib.__kgetStream()->memoryStream.memFree(stack0);
		__kprintf(ERROR TASK"0x%x: failed to alloc user stack.\n", id);
		return ERROR_MEMORY_NOMEM;
	};

	__kprintf(NOTICE"User stack allocated at 0x%p.\n", stack1);
	return ERROR_SUCCESS;
}

void taskC::initializeRegisterContext(
	void (*entryPoint)(void *), sarch_t isFirstThread
	)
{
	ubit8		stackIndex;

	if (parent->id == __KPROCESSID && isFirstThread)
	{
		panic(FATAL"SPAWNTHREAD_FLAGS_FIRST_THREAD used when spawning "
			"a kernel thread.\n");
	};

	if (parent->id == __KPROCESSID || isFirstThread)
	{
		/* Kernel threads always start on their kernel stack (and they
		 * have no userspace stack). Furthermore, the first thread of
		 * every non-kernel process actually executes in kernel domain
		 * for a short sequence, so the first thread of every new
		 * process begins executing in kernel-mode on its kernel stack.
		 **/
		stackIndex = 0;
	}
	else { stackIndex = 1; };

	if (stackIndex == 0)
	{
		context = (taskContextC *)((uintptr_t)stack0
			+ CHIPSET_MEMORY___KSTACK_NPAGES * PAGING_BASE_SIZE);
	}
	else
	{
		context = (taskContextC *)((uintptr_t)stack1
			+ CHIPSET_MEMORY_USERSTACK_NPAGES * PAGING_BASE_SIZE);
	};

	// Context -= sizeof(taskContextC);
	context--;

	/* For the very first thread in a process, we execute a short sequence
	 * of code in kernel mode, so if it's the first thread, we have to
	 * initialize it in PROCESS_EXECDOMAIN_KERNEL.
	 *
	 * For every subsequent thread in a process, we initialize the register
	 * context with the parent process' exec domain.
	 **/
	new (context) taskContextC(
		(isFirstThread)
			? PROCESS_EXECDOMAIN_KERNEL
			: parent->execDomain);

	context->initialize();
	context->setStacks(stack0, stack1, stackIndex);
	context->setEntryPoint(entryPoint);

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
				->taskStream.getCurrentTask()->cpuAffinity;
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
			.getCurrentTask()->schedPolicy;
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
			.getCurrentTask();

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

