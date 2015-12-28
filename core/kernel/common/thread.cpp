
#include <kernel/common/process.h>
#include <kernel/common/thread.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/processTrib/processTrib.h>


ubit8		_TaskContext::bspPowerTaskContextCpuAffinityMem[32];

sbit8 Thread::isBspPowerThread(processId_t tid)
{
	return isPowerThread(tid) && CpuStream::isBspCpuId(tid);
}


_Task::_Task(processId_t tid, Thread *parent, void *privateData)
:	Stream<Thread>(parent, tid),
privateData(privateData),
// Usually overridden immediately by inheritSchedPrio(), though.
schedPrio(&internalPrio),
internalPrio(CC"Custom", PRIOCLASS_DEFAULT),
schedPolicy(ROUND_ROBIN), schedOptions(0), schedFlags(0)
{
}

_TaskContext::_TaskContext(processId_t tid, Thread *parent)
: Stream<Thread>(parent, tid),
runState(UNSCHEDULED), blockState(BLOCKED_UNSCHEDULED),
context(NULL)
{
#if __SCALING__ >= SCALING_CC_NUMA
	defaultMemoryBank.rsrc = NUMABANKID_INVALID;
#endif

	memset(
		bspPowerTaskContextCpuAffinityMem, 0,
		sizeof(bspPowerTaskContextCpuAffinityMem));
}

Thread::Thread(
	processId_t id, ProcessStream *parent, void *privateData,
	CpuStream *parentCpu)
:	Stream<ProcessStream>(parent, id),
	_Task(id, this, privateData),
	_TaskContext(id, this),
id(id), parent(parent), region(NULL),
currentCpu(NULL), parentCpu(parentCpu),
stack0(NULL), stack1(NULL),
messageStream(this)
{
}

error_t _TaskContext::initialize(void)
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

	return ERROR_SUCCESS;
}

void _TaskContext::initializeRegisterContext(
	void (*entryPoint)(void *), void *stack0, void *stack1,
	ubit8 execDomain,
	sarch_t isFirstThread
	)
{
	ubit8		stackIndex;

	/**	EXPLANATION:
	 * This is used to initialize context on one of the stacks passed as
	 * stack0 or stack1.
	 *
	 * For kernel threads and power threads, only "stack0" will be passed,
	 * and stack1 will be NULL.
	 *
	 * For threads of processes other than the kernel, stack1 shall not be
	 * NULL, and it will be used as the context stack if it is not null.
	 * The only exception is if 'isFirstThread' is set. In that case, the
	 * context pointer will be initialized on stack0 instead and the
	 * register context will be built in the stack0 stack, because the first
	 * thread in every process actually begins running in kernel domain and
	 * not in user domain.
	 **/
	if (stack1 != NULL && !isFirstThread)
	{
		// Use stack1.
		stackIndex = 1;
		context = (RegisterContext *)((uintptr_t)stack1
			+ CHIPSET_MEMORY_USERSTACK_NPAGES * PAGING_BASE_SIZE);
	}
	else
	{
		/* Use stack0 for everything that is not a userspace thread,
		 * AND also not that userspace process' first thread.
		 **/
		stackIndex = 0;
		context = (RegisterContext *)((uintptr_t)stack0
			+ CHIPSET_MEMORY___KSTACK_NPAGES * PAGING_BASE_SIZE);
	};

	// Context -= sizeof(RegisterContext);
	context--;

	/* For the very first thread in a process, we execute a short sequence
	 * of code in kernel mode, so if it's the first thread, we have to
	 * initialize it in PROCESS_EXECDOMAIN_KERNEL.
	 *
	 * For every subsequent thread in a process, we initialize the register
	 * context with the parent process' exec domain.
	 **/
	new (context) RegisterContext(
		(isFirstThread)
			? PROCESS_EXECDOMAIN_KERNEL
			: execDomain);

	context->initialize();
	context->setStacks(stack0, stack1, stackIndex);
	context->setEntryPoint(entryPoint);
}

static inline error_t resizeAndMergeBitmaps(Bitmap *dest, Bitmap *src)
{
	error_t		ret;

	ret = dest->resizeTo(src->getNBits());
	if (ret != ERROR_SUCCESS) { return ret; };

	dest->merge(src);
	return ERROR_SUCCESS;
}

#if __SCALING__ >= SCALING_SMP
error_t _TaskContext::inheritAffinity(Bitmap *cpuAffinity, uarch_t flags)
{
	error_t		ret;

	/**	FIXME:
	 * There is an issue here for per-cpu threads where the CPU affinity
	 * bitmap is required for a proper memory allocation, but at the same
	 * time, this function is only executed on the hosting CPU.
	 *
	 * That means that before the cpuAffinity bitmap's internals are
	 * allocated, we need to allocate memory (to allocate the cpuAffinity
	 * bitmap's internals).
	 *
	 * The reason it isn't safe to allocate memory without a properly
	 * initialized cpuAffinity bitmap is that on a NUMA build, the kernel
	 * uses the cpuAffinity bitmap when choosing a NUMA node to allocate
	 * memory from for the current thread, if its defaultMemoryBank is
	 * NUMABANKID_INVALID or it has run out of pmem.
	 **/

	/**	EXPLANATION:
	 * Threads STINHERIT CPU Affinity by default. Overrides are:
	 * PARENT_CPU:
	 *	Forced if the thread is a power thread. Cannot be overridden.
	 *	Basically forces a power thread to be bound only to its parent
	 *	CPU, since they should never be migrated.
	 * PINHERIT:
	 *	Indicated by SPAWNTHREAD_FLAGS_AFFINITY_PINHERIT.
	 * SET:
	 *	Implied if cpuAffinity != NULL.
	 *
	 *	CAVEAT:
	 * For power-threads, cpuAffinity is always set to be ONLY the cpu
	 * itself. So for a power thread on CPU0, the cpuAffinity on
	 * that task context will only have bit 0 set (because we cannot allow
	 * power threads to migrate, obviously...).
	 **/
	if (Thread::isPowerThread(id))
	{
		ret = this->cpuAffinity.initialize(
			PROCID_THREAD(parent->getFullId()) + 1);

		if (ret != ERROR_SUCCESS) { return ret; };

		// Set the containing CPU's bit.
		this->cpuAffinity.setSingle(
			PROCID_THREAD(parent->getFullId()));

		return ERROR_SUCCESS;
	};

	if (cpuAffinity == NULL)
	{
		Bitmap		*sourceBitmap;

		// PINHERIT.
		if (FLAG_TEST(flags, SPAWNTHREAD_FLAGS_AFFINITY_PINHERIT)) {
			sourceBitmap = &parent->parent->cpuAffinity;
		}
		else
		{
			// Default: STINHERIT.
			/* A per-cpu thread cannot spawn a new per-cpu thread,
			 * since that would make it possible for a single CPU
			 * to have two per-cpu threads scheduled to it at once.
			 *
			 * And a unique-context thread cannot spawn a per-cpu
			 * thread either, because that will be prohibited by
			 * spawnPerCpuThread().
			 *
			 * And a per-cpu thread cannot spawn a unique-context
			 * thread because we prohibit that to reduce complexity.
			 * It is prohibited at the top of spawnThread().
			 *
			 * Therefore the only way we could be here is if a
			 * unique-context thread is spawning a new thread. We
			 * can safely assume therefore that the calling CPU's
			 * current thread is a unique-context thread.
			 **/
			sourceBitmap = &static_cast<Thread *>(
				cpuTrib.getCurrentCpuStream()->taskStream
					.getCurrentThread() )->cpuAffinity;
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

void _Task::inheritSchedPolicy(schedPolicyE schedPolicy, uarch_t /*flags*/)
{
	/**	EXPLANATION:
	 * Sched policy is STINHERITed by default. Overrides are:
	 * SET:
	 *	Taken if schedPolicy != 0.
	 **/
	if (schedPolicy != INVALID)
	{
		// SET.
		this->schedPolicy = schedPolicy;
	}
	else
	{
		// STINHERIT.
		this->schedPolicy = cpuTrib.getCurrentCpuStream()->taskStream
			.getCurrentThread()->schedPolicy;
	};
}

void _Task::inheritSchedPrio(prio_t prio, uarch_t flags)
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
	if (FLAG_TEST(flags, SPAWNTHREAD_FLAGS_SCHEDPRIO_SET))
	{
		FLAG_SET(this->flags, TASK_FLAGS_CUSTPRIO);
		internalPrio.prio = prio;
		schedPrio = &internalPrio;
		return;
	};

	if (FLAG_TEST(flags, SPAWNTHREAD_FLAGS_SCHEDPRIO_STINHERIT))
	{
		Thread		*spawningThread;

		spawningThread = cpuTrib.getCurrentCpuStream()->taskStream
			.getCurrentThread();

		if (FLAG_TEST(
			spawningThread->Stream<ProcessStream>::flags,
			TASK_FLAGS_CUSTPRIO))
		{
			FLAG_SET(this->flags, TASK_FLAGS_CUSTPRIO);
			internalPrio.prio = spawningThread->internalPrio.prio;
			schedPrio = &internalPrio;
		}
		else {
			schedPrio = spawningThread->schedPrio;
		};

		return;
	};

	if (FLAG_TEST(flags, SPAWNTHREAD_FLAGS_SCHEDPRIO_PRIOCLASS_SET)
		&& prio < PRIOCLASS_NCLASSES)
	{
		schedPrio = &prioClasses[prio];
		return;
	};

	schedPrio = &prioClasses[PRIOCLASS_DEFAULT];
	return;
}

error_t Thread::allocateStacks(void)
{
	/**	NOTES:
	 * There are 3 separate cases for consideration here:
	 *	1. Kernel threads: worker threads for the kernel itself, which
	 *	   are part of the kernel's process. These threads do not
	 *	   allocate a userspace stack.
	 *	2. Threads within a process that is not the kernel, but whose
	 *	   execdomain == PROCESS_EXECDOMAIN_KERNEL. Threads for such
	 *	   a process are not given a userspace stack, and are disallowed
	 *	   from doing so by kernel policy. Failure to comply will,
	 *	   depending on the CPU arch, likely result in kernel failure.
	 *	   These processes tend to be driver processes.
	 *	2. Threads for processes other than the kernel process, that are
	 *	   not PROCESS_EXECDOMAIN_KERNEL processes. These threads are
	 *	   given a userspace stack. This is the general case for almost
	 *	   every process.
	 *
	 * Basically, if the parent process' exec domain is KERNEL, we do not
	 * allocate a userspace stack, and furthermore, the use of a userspace
	 * stack for such a process is prohibited by kernel policy.
	 **/
	if (isPowerThread(getFullId()))
	{
		stack0 = parentCpu->taskStream.powerStack;
		return ERROR_SUCCESS;
	};

	// No fakemapping for kernel stacks.
	stack0 = processTrib.__kgetStream()->memoryStream.memAlloc(
		CHIPSET_MEMORY___KSTACK_NPAGES, MEMALLOC_NO_FAKEMAP);

	if (stack0 == NULL)
	{
		printf(ERROR TASK"0x%x: kernel stack alloc failed.\n", id);
		return ERROR_MEMORY_NOMEM;
	};

	// Don't allocate a user stack for kernel- and per-cpu- threads.
	if (parent->execDomain == PROCESS_EXECDOMAIN_KERNEL)
		{ return ERROR_SUCCESS; };

	// Allocate the stack from the parent process' memory stream.
	stack1 = parent->memoryStream.memAlloc(CHIPSET_MEMORY_USERSTACK_NPAGES);
	if (stack1 == NULL)
	{
		processTrib.__kgetStream()->memoryStream.memFree(stack0);
		printf(ERROR TASK"0x%x: failed to alloc user stack.\n", id);
		return ERROR_MEMORY_NOMEM;
	};

	return ERROR_SUCCESS;
}

