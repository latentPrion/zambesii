
#include <scaling.h>
#include <arch/cpuControl.h>
#include <arch/registerContext.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/process.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/taskTrib/taskStream.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <__kthreads/__kcpuPowerOn.h>
#include <__kthreads/main.h>


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

TaskStream::TaskStream(cpu_t cid, CpuStream *parent)
: Stream<CpuStream>(parent, cid),
load(0), capacity(0),
currentThread(&powerThread),
roundRobinQ(SCHEDPRIO_MAX_NPRIOS), realTimeQ(SCHEDPRIO_MAX_NPRIOS),
powerThread(cid, processTrib.__kgetStream(), NULL, parent)
{
	// Set the power thread's current CPU pointer.
	powerThread.currentCpu = parent;

	if (!CpuStream::isBspCpuId(id)) {
		memset(powerStack, 0, sizeof(powerStack));
	};
}

error_t TaskStream::initialize(void)
{
	error_t		ret;

	ret = realTimeQ.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	ret = roundRobinQ.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	ret = powerThread.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	return ERROR_SUCCESS;
}

void TaskStream::dump(void)
{
	printf(NOTICE TASKSTREAM"%d: load %d, capacity %d, "
		"currentTask 0x%x(@0x%p): dump.\n",
		parent->cpuId,
		load, capacity,
		getCurrentThread()->getFullId(),
		getCurrentThread());

	realTimeQ.dump();
	roundRobinQ.dump();
}

error_t TaskStream::cooperativeBind(void)
{
	error_t		ret;
	Thread		*dummy;

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

	ret = processTrib.__kgetStream()->spawnThread(
		NULL, &powerThread,
		NULL, Thread::ROUND_ROBIN, 0,
		SPAWNTHREAD_FLAGS_POWER_THREAD,
		&dummy);

	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR TASKSTREAM"%d: coopBind: spawnThread() failed for "
			"power thread.\n",
			this->id);

		return ret;
	};

	cpuTrib.onlineCpus.setSingle(parent->cpuId);

	/**	CAVEAT:
	 * We do /NOT/ schedule() the power thread, since it should be the
	 * thread we are /ALREADY/ executing in at this point. When a thread
	 * is executing, it should not be in the runqueue.
	 *
	 * If we schedule() the power thread when it is executing, we would,
	 * in effect, re-add it to to the runqueue such that it would exist
	 * in the runqueue twice.
	 *
	 * Consider:
	 *	schedule() => { Now power thread is in currentThread, and in the
	 * 		runqueue }
	 *	yield() => { Now power thread gets added again to runqueue }
	 *	pull() x2 => { Power thread which is listed twice, will get
	 * 		pulled twice }
	 **/
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
		&thread->parent->cpuTrace, parent->cpuId, &ret,
		"schedule", "cpuTrace");

	if (ret != ERROR_SUCCESS) { return ret; };
#endif
	// TODO: Check CPU suitability to run task (FPU, other features).
	// TODO: Validate any scheduling parameters that need to be checked.

	thread->runState = Thread::RUNNABLE;
	thread->currentCpu = parent;

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

	thread->parent->cpuTrace.setSingle(parent->cpuId);
	// Increment and notify upper layers of new task being scheduled.
	updateLoad(LOAD_UPDATE_ADD, 1);
	return ret;
}

#include <debug.h>
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
		if (/* !parent->isBspCpu() */ 0)
		{
			printf(NOTICE TASKSTREAM"%d: Entering C1.\n",
				parent->cpuId);
		};

		cpuControl::halt();
	};

	// FIXME: Should take a tlbContextC object instead.
	tlbControl::saveContext(
		&currentThread->parent->getVaddrSpaceStream()->vaddrSpace);

	/**	TODO
	 * Add a way for the kernel to support having a particular thread be
	 * the current thread, but also not have that thread's process'
	 * addrspace be loaded. This would be necessary to enable the kernel
	 * to avoid having to load the kernel's addrspace when switching to a
	 * kernel thread.
	 *
	 * It should be possible to switch to a kernel thread without having to
	 * switch out of whichever addrspace was loaded before, since the kernel
	 * is mapped into every process.
	 **/
	if (newThread->parent->getVaddrSpaceStream()
		!= currentThread->parent->getVaddrSpaceStream()
		/*&& newThread->parent->id != __KPROCESSID*/)
	{
		tlbControl::loadContext(
			&newThread->parent->getVaddrSpaceStream()
				->vaddrSpace);
	};

	newThread->runState = Thread::RUNNING;
	currentThread = newThread;

printf(NOTICE TASKSTREAM"%d: Switching to task 0x%x.\n",
	parent->cpuId, newThread->getFullId());

	loadContextAndJump(newThread->context);
}

Thread* TaskStream::pullFromQ(utf8Char *which)
{
	Thread			*ret;
	status_t		status;
	PrioQueue<Thread>	*queue=NULL;

	(void)status;

	if (!strcmp8(which, CC"realtime")) { queue = &realTimeQ; };
	if (!strcmp8(which, CC"roundrobin")) { queue = &roundRobinQ; };
	if (queue == NULL) { return NULL; };

	for (;FOREVER;)
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
	};
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

	ncb = cpuTrib.getBank(parent->bankId);
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

	ncb = cpuTrib.getBank(parent->bankId);
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

