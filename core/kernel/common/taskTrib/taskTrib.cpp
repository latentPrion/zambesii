
#include <scaling.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/numaCpuBank.h>
#include <kernel/common/panic.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


TaskTrib::TaskTrib(void)
{
	capacity = 0;
	load = 0;
}

#if __SCALING__ >= SCALING_SMP
error_t TaskTrib::schedule(Thread *task)
{
	CpuStream	*cs, *bestCandidate=NULL;

	for (cpu_t i=0; i<task->cpuAffinity.getNBits(); i++)
	{
		if (task->cpuAffinity.testSingle(i)
			&& cpuTrib.onlineCpus.testSingle(i))
		{
			cs = cpuTrib.getStream(i);

			if (bestCandidate == NULL)
			{
				bestCandidate = cs;
				continue;
			};

			if (cs->taskStream.getLoad()
				< bestCandidate->taskStream.getLoad())
			{
				bestCandidate = cs;
			};
		};
	};

	if (bestCandidate == NULL) { return ERROR_UNKNOWN; };
	return bestCandidate->taskStream.schedule(task);
}
#endif

void TaskTrib::updateCapacity(ubit8 action, uarch_t val)
{
	switch (action)
	{
	case CAPACITY_UPDATE_ADD:
		capacity += val;
		return;

	case CAPACITY_UPDATE_SUBTRACT:
		capacity -= val;
		return;

	case CAPACITY_UPDATE_SET:
		capacity = val;
		return;

	default: return;
	};
}

void TaskTrib::updateLoad(ubit8 action, uarch_t val)
{
	switch (action)
	{
	case LOAD_UPDATE_ADD:
		load += val;
		return;

	case LOAD_UPDATE_SUBTRACT:
		load -= val;
		return;

	case LOAD_UPDATE_SET:
		load = val;
		return;

	default: return;
	};
}

error_t TaskTrib::dormant(Thread *thread)
{
	CpuStream	*threadCurrentCpu;

	/**	FIXME:
	 * The following inconsistent behaviour arises:
	 *
	 * If a thread, T1, running on CPU1 calls to have a thread, T2,
	 * currently being executed on CPU2, dormant()ed, the call will
	 * go through successfully and T2 will be removed from CPU2's runqueue
	 * at first;
	 *
	 * However, T2 is still being executed on CPU2, and it will not stop
	 * being executed until (1) T2's time quantum expires, (2) T2 is made
	 * to block, (3) T2 deliberately also calls dormant() on itself, or
	 * (4) we send an IPI to CPU2 telling it to stop executing T2 and
	 * forcibly make CPU2 pre-empt T2.
	 *
	 * However, in every one of those cases except for (3), the act that
	 * eventually causes T2 to stop executing will also either re-insert
	 * T2 into the runqueue on CPU2 or in the case of (2) cause T2 to block
	 * on some resource and eventually be re-inserted into the runqueue
	 * in the end anyway.
	 *
	 * In other words, right now, if dormant() is called by a foreign thread
	 * attempting to act on a thread other than itself, the call will
	 * successfully execute, but the expected behaviour will silently be
	 * disobeyed.
	 *
	 * The expected behaviour of dormant() is to effectively halt the thread
	 * indefinitely until another thread calls wake() on the dormanted
	 * thread. In every case from above, the thread would be re-inserted
	 * into the runqueue at some point and eventually be re-pulled and
	 * have its execution resumed.
	 **/

	if (thread == NULL) { return ERROR_INVALID_ARG; };

	if (thread->runState == Thread::UNSCHEDULED
		|| (thread->runState == Thread::STOPPED
			&& thread->blockState == Thread::DORMANT))
	{
		return ERROR_SUCCESS;
	};

	thread->currentCpu->taskStream.dormant(thread);
	threadCurrentCpu = thread->currentCpu;

	/* At this point, taskCurrentCpu points to the CPU which the
	 * particular unique thread or per-cpu thread that was our target, is
	 * currently scheduled to. For a per-cpu thread, it points to the
	 * particular CPU whose per-cpu thread was acted on.
	 **/
	if (threadCurrentCpu != cpuTrib.getCurrentCpuStream())
	{
		/* Message the foreign CPU to preempt and choose another thread.
		 * Should probably take an argument for the thread to be
		 * pre-empted.
		 **/
	}
	else
	{
		/* If the task is both on the current CPU and currently being
		 * executed, force the current CPU to context switch.
		 **/
		if (thread == cpuTrib.getCurrentCpuStream()->taskStream
			.getCurrentThread())
		{
			// TODO: Set this CPU's currentTask to NULL here.
			saveContextAndCallPull(
				&threadCurrentCpu->schedStack[
					sizeof(threadCurrentCpu->schedStack)]);

			// Unreachable.
		};
	};

	return ERROR_SUCCESS;
}

error_t TaskTrib::wake(Thread *thread)
{
	error_t		err;

	if (thread == NULL) { return ERROR_INVALID_ARG; };

	if (thread->runState != Thread::UNSCHEDULED
		&& (thread->runState != Thread::STOPPED
			&& thread->blockState != Thread::DORMANT))
	{
		return ERROR_SUCCESS;
	};

	if (thread->runState == Thread::UNSCHEDULED) {
		err = schedule(thread);
	}
	else { err = thread->currentCpu->taskStream.wake(thread); };

	if (err != ERROR_SUCCESS) {
		panic(err, ERROR"Failed to wake thread!");
	}

	if (thread->shouldPreemptCurrentThreadOn(thread->currentCpu))
	{
		if (thread->currentCpu == cpuTrib.getCurrentCpuStream())
			{ printf(CC"About to yield...\n"); yield(); }
		else
		{
			/* FIXME: Send an IPC message to the other CPU to check
		 	* its queue in case the newly scheduled thread is higher
		 	* priority.
		 	*/
		}
	}

	return err;
}

void TaskTrib::yield(void)
{
	Thread		*currThread;

	/* Yield() and block() are always called by the current thread, and
	 * they always act on the thread that called them.
	 **/
	currThread = cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentThread();

	cpuTrib.getCurrentCpuStream()->taskStream.yield(currThread);

	// TODO: Set this CPU's currentTask to NULL here.
	saveContextAndCallPull(
		&cpuTrib.getCurrentCpuStream()->schedStack[
			sizeof(cpuTrib.getCurrentCpuStream()->schedStack)]);
}

void TaskTrib::block(Lock::sOperationDescriptor *unlockDescriptor)
{
	Thread		*currThread;

	/* Yield() and block() are always called by the current thread, and
	 * they always act on the thread that called them.
	 **/

	/**	XXX:
	 * If you update this function, be sure to update its overloaded version
	 * above as well.
	 *
	 *	NOTES:
	 * After placing a task into a waitqueue, call this function to
	 * place it into a "blocked" state.
	 **/
	currThread = cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentThread();

	cpuTrib.getCurrentCpuStream()->taskStream.block(currThread);

	/**	EXPLANATION:
	 * This bit here completely purges the problem of lost wakeups
	 * for asynchronous callbacks and event notifications. There is a window
	 * between the point when a thread checks a callback/event message queue
	 * (and finds the queue empty), and the point when the thread is removed
	 * from the scheduler queues due to the resultant call to block()
	 * (because the queue was empty).
	 *
	 * If a new callback/event message is inserted at this time, the call
	 * to unblock() the thread (by the sender of the callback/event) may
	 * arrive before the thread has actually removed itself from the
	 * scheduler queues.
	 *
	 * If this happens, the thread will remove itself from the scheduler
	 * queues /after/ the call to unblock() was made by the callback/event
	 * sender, (when it should have happened before), and therefore the
	 * thread will never wake again.
	 *
	 * This next bit of code is part of the solution: a call to block() can
	 * be made while the thread holds a lock (which should prevent any new
	 * callbacks/events from being queued until it is released), and
	 * this next bit of code will release the lock immediately upon removing
	 * the thread from the scheduler queues; thereby eliminating the race
	 * condition.
	 *
	 * Be sure to pass in the correct 'lockType' value, and for multiple-
	 * reader locks, the correct 'unlockOp' value.
	 **/
	if (unlockDescriptor != NULL) { unlockDescriptor->execute(); };

	// TODO: Set this CPU's currentTask to NULL here.
	saveContextAndCallPull(
		&cpuTrib.getCurrentCpuStream()->schedStack[
			sizeof(cpuTrib.getCurrentCpuStream()->schedStack)]);
}

static utf8Char		*runStates[5] =
{
	CC"invalid", CC"unscheduled", CC"runnable", CC"running", CC"stopped"
};

error_t TaskTrib::unblock(Thread *thread)
{
	if (thread == NULL) { return ERROR_INVALID_ARG; };

	if (thread->runState == Thread::RUNNABLE
		|| thread->runState == Thread::RUNNING)
	{
		return ERROR_SUCCESS;
	};

	if (!(thread->runState == Thread::STOPPED
		&& thread->blockState == Thread::BLOCKED))
	{
		printf(NOTICE TASKTRIB"unblock(%x): Invalid run state."
			"runState is %s.\n",
			thread->getFullId(),
			runStates[thread->runState]);

		return ERROR_INVALID_OPERATION;
	};

	return thread->currentCpu->taskStream.unblock(thread);
}

