
#include <scaling.h>
#include <arch/cpuControl.h>
#include <arch/registerContext.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/process.h>
#include <kernel/common/taskTrib/taskStream.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <__kthreads/__kcpuPowerOn.h>
#include <__kthreads/__korientation.h>


extern "C" void taskStream_pull(RegisterContext *savedContext)
{
	TaskStream	*currTaskStream;

	// XXX: We are operating on the CPU's sleep stack.
	currTaskStream = &cpuTrib.getCurrentCpuStream()->taskStream;
	if (savedContext != NULL) {
		currTaskStream->getCurrentThread()->context = savedContext;
	};

	currTaskStream->pull();
}

TaskStream::TaskStream(CpuStream *parent)
:
Stream(0),
load(0), capacity(0),
roundRobinQ(SCHEDPRIO_MAX_NPRIOS), realTimeQ(SCHEDPRIO_MAX_NPRIOS),
parentCpu(parent)
{
	/* Ensure that the BSP CPU's pointer to __korientation isn't trampled
	 * by the general case constructor.
	 **/
	if (!FLAG_TEST(parentCpu->flags, CPUSTREAM_FLAGS_BSP)) {
		currentThread = &__kcpuPowerOnThread;
	};
}

error_t TaskStream::initialize(void)
{
	error_t		ret;

	ret = realTimeQ.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	return roundRobinQ.initialize();
}

void TaskStream::dump(void)
{
	printf(NOTICE TASKSTREAM"%d: load %d, capacity %d, "
		"currentTask 0x%x(@0x%p): dump.\n",
		parentCpu->cpuId,
		load, capacity,
		getCurrentThread()->getFullId(),
		getCurrentThread());

	realTimeQ.dump();
	roundRobinQ.dump();
}

error_t TaskStream::cooperativeBind(void)
{
	/**	EXPLANATION:
	 * This function is only ever called on the BSP CPU's Task Stream,
	 * because only the BSP task stream will ever be deliberately co-op
	 * bound without even checking for the presence of pre-empt support.
	 *
	 * At boot, co-operative scheduling is needed to enable certain kernel
	 * services to run, such as the timer services, etc. The timer trib
	 * service for example uses a worker thread to dispatch timer queue
	 * request objects. There must be a simple scheduler in place to enable
	 * thread switching.
	 *
	 * Thus, here we are. To enable co-operative scheduling, we simply
	 * bring up the BSP task stream, add the __korientation thread to it,
	 * set the CPU's bit in onlineCpus, then exit.
	 **/
	/*__korientationThread.schedPolicy = Task::ROUND_ROBIN;
	__korientationThread.schedOptions = 0;
	__korientationThread.schedFlags = 0;

	return roundRobinQ.insert(
		&__korientationThread,
		__korientationThread.schedPrio->prio,
		__korientationThread.schedOptions);*/

	cpuTrib.onlineCpus.setSingle(parentCpu->cpuId);
	return ERROR_SUCCESS;
}

status_t TaskStream::schedule(Thread *thread)
{
	status_t	ret;

	/**	EXPLANATION:
	 * TaskStreamC::schedule() is the lowest level schedule() call; it is
	 * per-CPU in nature and it is the only schedule() layer that can
	 * accept a per-cpu thread as an argument since per-cpu threads are
	 * directly scheduled to their target CPUs (using this function).
	 *
	 * For a normal thread, we just schedule it normally. For per-cpu
	 * threads, we also have to set the CPU's currentPerCpuThread pointer.
	 **/
#if __SCALING__ >= SCALING_SMP
	CHECK_AND_RESIZE_BMP(
		&thread->parent->cpuTrace, parentCpu->cpuId, &ret,
		"schedule", "cpuTrace");

	if (ret != ERROR_SUCCESS) { return ret; };
#endif
	// TODO: Check CPU suitability to run task (FPU, other features).
	// TODO: Validate any scheduling parameters that need to be checked.

	thread->runState = Thread::RUNNABLE;
	thread->currentCpu = parentCpu;

	// Finally, add the task to a queue.
	switch (thread->schedPolicy)
	{
	case Thread::ROUND_ROBIN:
		ret = roundRobinQ.insert(
			thread, thread->schedPrio->prio, thread->schedOptions);

		break;

	case Thread::REAL_TIME:
		ret = realTimeQ.insert(
			thread, thread->schedPrio->prio, thread->schedOptions);

		break;

	default:
		return ERROR_INVALID_ARG_VAL;
	};

	if (ret != ERROR_SUCCESS) { return ret; };

	thread->parent->cpuTrace.setSingle(parentCpu->cpuId);
	// Increment and notify upper layers of new task being scheduled.
	updateLoad(LOAD_UPDATE_ADD, 1);
	return ret;
}

static inline void getCr3(paddr_t *ret)
{
	asm volatile("movl	%%cr3, %0\n\t"
		: "=r" (*ret));
}

void TaskStream::pull(void)
{
	Thread		*newThread;

	for (;FOREVER;)
	{
		newThread = pullFromQ(CC"realtime");
		if (newThread != NULL) {
			break;
		};

		newThread = pullFromQ(CC"roundrobin");
		if (newThread != NULL) {
			break;
		};

		// Else set the CPU to a low power state.
		if (/*!FLAG_TEST(parentCpu->flags, CPUSTREAM_FLAGS_BSP)*/ 0)
		{
			printf(NOTICE TASKSTREAM"%d: Entering C1.\n",
				parentCpu->cpuId);
		};

		cpuControl::halt();
	};

	// FIXME: Should take a tlbContextC object instead.
	tlbControl::saveContext(
		&currentThread->parent->getVaddrSpaceStream()->vaddrSpace);

	// If addrspaces differ, switch; don't switch if kernel/per-cpu thread.
	if (newThread->parent->getVaddrSpaceStream()
		!= currentThread->parent->getVaddrSpaceStream()
		&& newThread->parent->id != __KPROCESSID)
	{
		tlbControl::loadContext(
			&newThread->parent->getVaddrSpaceStream()
				->vaddrSpace);
	};

	newThread->runState = Thread::RUNNING;
	currentThread = newThread;
printf(NOTICE TASKSTREAM"%d: Switching to task 0x%x.\n",
	parentCpu->cpuId, newThread->getFullId());
	loadContextAndJump(newThread->context);
}

// TODO: Merge the two queue pull functions for cache efficiency.
Thread* TaskStream::pullFromQ(utf8Char *which)
{
	Thread			*ret;
	status_t		status;
	PrioQueue<Thread>	*queue=NULL;

	if (!strcmp8(which, CC"realtime")) { queue = &realTimeQ; };
	if (!strcmp8(which, CC"roundrobin")) { queue = &roundRobinQ; };
	if (queue == NULL) { return NULL; };

	(void)status;
	do
	{
		ret = queue->pop();
		if (ret == NULL) {
			return NULL;
		};

		// Make sure the scheduler isn't waiting for this task.
		if (FLAG_TEST(
			ret->schedFlags, TASK_SCHEDFLAGS_SCHED_WAITING))
		{
			status = schedule(ret);
			continue;
		};

		return ret;
	} while (1);
}

void TaskStream::updateCapacity(ubit8 action, uarch_t val)
{
	NumaCpuBank		*ncb;

	switch (action)
	{
	case CAPACITY_UPDATE_ADD:
		capacity += val;
		break;

	case CAPACITY_UPDATE_SUBTRACT:
		capacity -= val;
		break;

	case CAPACITY_UPDATE_SET:
		capacity = val;
		break;

	default: return;
	};

	ncb = cpuTrib.getBank(parentCpu->bankId);
	if (ncb == NULL) { return; };

	ncb->updateCapacity(action, val);
}

void TaskStream::updateLoad(ubit8 action, uarch_t val)
{
	NumaCpuBank		*ncb;

	switch (action)
	{
	case LOAD_UPDATE_ADD:
		load += val;
		break;

	case LOAD_UPDATE_SUBTRACT:
		load -= val;
		break;

	case LOAD_UPDATE_SET:
		load = val;
		break;

	default: return;
	};

	ncb = cpuTrib.getBank(parentCpu->bankId);
	if (ncb == NULL) { return; };

	ncb->updateLoad(action, val);
}

void TaskStream::dormant(Thread *thread)
{
	thread->runState = Thread::STOPPED;
	thread->blockState = Thread::DORMANT;

	switch (thread->schedPolicy)
	{
	case Thread::ROUND_ROBIN:
		roundRobinQ.remove(thread, thread->schedPrio->prio);
		break;

	case Thread::REAL_TIME:
		realTimeQ.remove(thread, thread->schedPrio->prio);
		break;

	default: // Might want to panic or make some noise here.
		return;
	};
}

void TaskStream::block(Thread *thread)
{
	thread->runState = Thread::STOPPED;
	thread->blockState = Thread::BLOCKED;

	switch (thread->schedPolicy)
	{
	case Thread::ROUND_ROBIN:
		roundRobinQ.remove(thread, thread->schedPrio->prio);
		break;

	case Thread::REAL_TIME:
		realTimeQ.remove(thread, thread->schedPrio->prio);
		break;

	default:
		return;
	};
}

error_t TaskStream::unblock(Thread *thread)
{
	thread->runState = Thread::RUNNABLE;

	switch (thread->schedPolicy)
	{
	case Thread::ROUND_ROBIN:
		return roundRobinQ.insert(
			thread, thread->schedPrio->prio, thread->schedOptions);

	case Thread::REAL_TIME:
		return realTimeQ.insert(
			thread, thread->schedPrio->prio,
			thread->schedOptions);

	default:
		return ERROR_UNSUPPORTED;
	};
}

void TaskStream::yield(Thread *thread)
{
	thread->runState = Thread::RUNNABLE;

	switch (thread->schedPolicy)
	{
	case Thread::ROUND_ROBIN:
		roundRobinQ.insert(
			thread, thread->schedPrio->prio,
			thread->schedOptions);
		break;

	case Thread::REAL_TIME:
		realTimeQ.insert(
			thread, thread->schedPrio->prio,
			thread->schedOptions);
		break;

	default:
		return;
	};
}

error_t TaskStream::wake(Thread *thread)
{
	if (!(thread->runState == Thread::STOPPED
		&& thread->blockState == Thread::DORMANT)
		&& thread->runState != Thread::UNSCHEDULED)
	{
		return ERROR_UNSUPPORTED;
	};

	thread->runState = Thread::RUNNABLE;

	switch (thread->schedPolicy)
	{
	case Thread::ROUND_ROBIN:
		return roundRobinQ.insert(
			thread, thread->schedPrio->prio, thread->schedOptions);

	case Thread::REAL_TIME:
		return realTimeQ.insert(
			thread, thread->schedPrio->prio, thread->schedOptions);

	default:
		return ERROR_CRITICAL;
	};
}

