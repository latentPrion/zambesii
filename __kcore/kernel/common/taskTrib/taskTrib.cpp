
#include <scaling.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/numaCpuBank.h>
#include <kernel/common/panic.h>
#include <kernel/common/multipleReaderLock.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


TaskTrib::TaskTrib(void)
:
deadQ(CC"TaskTrib deadQ")
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

	MultipleReaderLock::ScopedWriteGuard	guard(&thread->schedState.lock);

	/** EXPLANATION:
	 * This current implementation of dormant() will leave an unscheduled
	 * thread in the UNSCHEDULED state if invoked on one. Basically, calling
	 * dormant() on an unscheduled thread is a no-op.
	 **/
	if (thread->schedState.rsrc.status == Thread::UNSCHEDULED
		|| thread->schedState.rsrc.status == Thread::DORMANT)
		{ return ERROR_SUCCESS; };

	thread->schedState.rsrc.currentCpu->taskStream.dormant(thread);

	/* At this point, taskCurrentCpu points to the CPU which the
	 * particular unique thread or per-cpu thread that was our target, is
	 * currently scheduled to. For a per-cpu thread, it points to the
	 * particular CPU whose per-cpu thread was acted on.
	 **/
	if (thread->schedState.rsrc.currentCpu != cpuTrib.getCurrentCpuStream())
	{
		guard.releaseManagementAndUnlock();

		/* Message the foreign CPU to preempt and choose another thread.
		 * Should probably take an argument for the thread to be
		 * pre-empted.
		 **/
	}
	else
	{
		guard.releaseManagementAndUnlock();

		/* If the task is both on the current CPU and currently being
		 * executed, force the current CPU to context switch.
		 **/
		if (thread == cpuTrib.getCurrentCpuStream()->taskStream
			.getCurrentThread())
		{
			CpuStream	*currCpu;

			currCpu = cpuTrib.getCurrentCpuStream();
if (currCpu->asyncInterruptEvent.getNestingLevel() > 0) {printf(FATAL TASKTRIB"In int context: asyncNestingLevel is %d.\n", currCpu->asyncInterruptEvent.getNestingLevel());}
			// TODO: Set this CPU's currentTask to NULL here.
			saveContextAndCallPull(
				&currCpu->schedStack[
					sizeof(currCpu->schedStack)],
				currCpu->asyncInterruptEvent.getNestingLevel());
		};
	};

	return ERROR_SUCCESS;
}

error_t TaskTrib::wake(Thread *thread)
{
	error_t		err;

	if (thread == NULL) { return ERROR_INVALID_ARG; };

	MultipleReaderLock::ScopedWriteGuard	guard(&thread->schedState.lock);

	if (thread->schedState.rsrc.status == Thread::RUNNABLE
		|| thread->schedState.rsrc.status == Thread::RUNNING)
		{ return ERROR_SUCCESS; };

	if (thread->schedState.rsrc.status == Thread::UNSCHEDULED)
	{
		printf(FATAL TASKTRIB"wake(%x): called on an unscheduled "
			"thread. Please schedule() it first.\n",
			thread->getFullId());

		return ERROR_INVALID_STATE;
	}

	err = thread->schedState.rsrc.currentCpu->taskStream.wake(thread);
	if (err != ERROR_SUCCESS) {
		panic(err, ERROR"wake() failed on thread!");
	}

printf(FATAL TASKTRIB"wake(%x): about to check if thread should preempt current thread.\n",
	thread->getFullId());
	if (thread->shouldPreemptCurrentThreadOn(
		thread->schedState.rsrc.currentCpu))
	{
printf(FATAL TASKTRIB"wake(%x): thread should preempt current thread.\n",
	thread->getFullId());
		if (thread->schedState.rsrc.currentCpu == cpuTrib.getCurrentCpuStream())
		{
			guard.releaseManagementAndUnlock();

			/** EXPLANATION:
			 * We shouldn't perform an immediate task switch here because
			 * * If we're on a coop sched build, the user expects not to
			 *   be pre-empted merely because a higher priority thread has
			 *   been woken up.
			 * * If we're on a passive preempt build, the preemptor will enforce
			 *   the preemption at kernel exit.
			 * * Wake(), by its nature, is not intended to support an immediate
			 *   task switch, because it doesn't put the current thread back
			 *   into the runqueue.
			 *
			 * So, we just return here and let the next kernel exit do the
			 * task switch.
			 **/
		}
		else
		{
			guard.releaseManagementAndUnlock();

			/** EXPLANATION:
			 * We're on a different CPU, so we need to send an IPC message
			 * to the other CPU to check if it has a higher priority thread
			 * in its runqueue.
			 **/
		}
	}

	return err;
}

void TaskTrib::yield(void)
{
	Thread		*currThread;
	CpuStream	*currCpuStream;

	/* Yield() and block() are always called by the current thread, and
	 * they always act on the thread that called them.
	 **/
	currCpuStream = cpuTrib.getCurrentCpuStream();
	currThread = currCpuStream->taskStream.getCurrentThread();

#ifdef CONFIG_DEBUG_SCHEDULER
	/* All threads should be in the RUNNING state when calling yield().
	 * Power threads are set to RUNNING state in their constructors.
	 **/
printf(FATAL TASKTRIB"In yield(): currThread->id is %x, schedState is %d(%s) (addr %p).\n",
	currThread->getFullId(), currThread->schedState.rsrc.status, Thread::schedStates[currThread->schedState.rsrc.status], &currThread->schedState);
	assert_fatal(currThread->schedState.rsrc.status == Thread::RUNNING);
#endif

	{
		MultipleReaderLock::ScopedWriteGuard	guard(
			&currThread->schedState.lock);

		currCpuStream->taskStream.yield(currThread);
	}

if (currCpuStream->asyncInterruptEvent.getNestingLevel() > 0) {printf(FATAL TASKTRIB"In int context: asyncNestingLevel is %d.\n", currCpuStream->asyncInterruptEvent.getNestingLevel());}
	// TODO: Set this CPU's currentTask to NULL here.
	saveContextAndCallPull(
		&currCpuStream->schedStack[
			sizeof(currCpuStream->schedStack)],
		currCpuStream->asyncInterruptEvent.getNestingLevel());
}

void TaskTrib::block(Lock::sOperationDescriptor *unlockDescriptor)
{
	Thread		*currThread;
	CpuStream	*currCpuStream;

	/**	NOTES:
	 * After placing a task into a waitqueue, call this function to
	 * place it into a "blocked" state.
	 **/

	/* Yield() and block() are always called by the current thread, and
	 * they always act on the thread that called them.
	 **/
	currCpuStream = cpuTrib.getCurrentCpuStream();
	currThread = currCpuStream->taskStream.getCurrentThread();

	if (currCpuStream->asyncInterruptEvent.getNestingLevel() > 0) {printf(FATAL TASKTRIB"In int context: asyncNestingLevel is %d.\n", currCpuStream->asyncInterruptEvent.getNestingLevel());}
	assert_fatal(currCpuStream->asyncInterruptEvent.getNestingLevel() == 0);

#ifdef CONFIG_DEBUG_SCHEDULER
	/* All threads should be in the RUNNING state when calling block().
	 * Power threads are set to RUNNING state in their constructors.
	 **/
if (currThread->schedState.rsrc.status != Thread::RUNNING) {
printf(FATAL TASKTRIB"In block(): currThread->id is %x, schedState is %d(%s) (addr %p).\n",
	currThread->getFullId(), currThread->schedState.rsrc.status,
	Thread::schedStates[currThread->schedState.rsrc.status],
	&currThread->schedState);
}
	assert_fatal(currThread->schedState.rsrc.status == Thread::RUNNING);
#endif

	if (unlockDescriptor == NULL)
	{
		MultipleReaderLock::ScopedWriteGuard	guard(
			&currThread->schedState.lock);

		currCpuStream->taskStream.block(currThread);
	}
	else {
		currCpuStream->taskStream.block(currThread);
	}

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
		&currCpuStream->schedStack[
			sizeof(currCpuStream->schedStack)],
		currCpuStream->asyncInterruptEvent.getNestingLevel());
}

error_t TaskTrib::unblock(
	Thread *thread,
	Lock::sOperationDescriptor *unlockDescriptor
	)
{
	error_t					err;
	MultipleReaderLock::ScopedReadGuard	schedStateGuard;
	
	// Tracing control for this function
	const bool enableTracing = false;

	if (thread == NULL) { return ERROR_INVALID_ARG; };

	/** NOTE:
	 * It's permissible for a thread to call unblock() on itself, because
	 * it may have called block() on itself outside of IRQ context, and
	 * then an IRQ may have occured (running in that same thread's context)
	 * which called unblock() on the thread. Just documenting for posterity.
	 **/

#if 0
	if (callerSchedStateGuard != NULL) {
		schedStateGuard.move_assign(*callerSchedStateGuard);
	}
	else
	{
		new (&schedStateGuard) MultipleReaderLock::ScopedReadGuard(
			&thread->schedState.lock);
	}
#endif

	if (enableTracing) {
printf(CC"{1}");
	}
	if (thread->schedState.rsrc.status == Thread::RUNNABLE
		|| thread->schedState.rsrc.status == Thread::RUNNING)
	{
		if (enableTracing) {
printf(CC"{1.1}");
		}
		/* It's not the worst thing if this happens, so just warn the user
		 * that their state machine is flawed and move on.
		 */
		printf(WARNING TASKTRIB"unblock(%x), CPU %d: Thread is already "
			"running or runnable (schedState is %d (%s)).\n",
			thread->getFullId(),
			cpuTrib.getCurrentCpuStream()->cpuId,
			thread->schedState.rsrc.status,
			Thread::schedStates[thread->schedState.rsrc.status]);

		if (unlockDescriptor != NULL) { unlockDescriptor->execute(); }

		return ERROR_SUCCESS;
	};

	if (enableTracing) {
printf(CC"{2: %d(%s)}\n", thread->schedState.rsrc.status, Thread::schedStates[thread->schedState.rsrc.status]);
	}
	if (thread->schedState.rsrc.status != Thread::BLOCKED)
	{
		if (enableTracing) {
printf(CC"{2.1}");
		}
		printf(NOTICE TASKTRIB"unblock(%x): Invalid sched state."
			"schedState is %d (%s).\n",
			thread->getFullId(),
			thread->schedState.rsrc.status,
			Thread::schedStates[thread->schedState.rsrc.status]);

		if (unlockDescriptor != NULL) { unlockDescriptor->execute(); }

		return ERROR_INVALID_OPERATION;
	};

	if (enableTracing) {
printf(CC"{3}");
	}
	err = thread->schedState.rsrc.currentCpu->taskStream.unblock(thread);
	if (unlockDescriptor != NULL) { unlockDescriptor->execute(); }

	if (err != ERROR_SUCCESS)
		{ panic(err, ERROR"Failed to unblock thread!"); }
	if (enableTracing) {
printf(CC"{4}");
	}

	if (thread->shouldPreemptCurrentThreadOn(
		thread->schedState.rsrc.currentCpu))
	{
		if (enableTracing) {
printf(CC"{5}");
		}
		if (thread->schedState.rsrc.currentCpu == cpuTrib.getCurrentCpuStream())
		{
//			schedStateGuard.releaseManagementAndUnlock();
			if (enableTracing) {
printf(CC"{6}");
			}
			/* We don't perform an immediate task switch here for the same
			 * reasons as in the wake() function. See comments in that function
			 * for more details.
			 **/
		}
		else
		{
//			schedStateGuard.releaseManagementAndUnlock();
			if (enableTracing) {
printf(CC"{7}");
			}
			/* FIXME: Send an IPC message to the other CPU to check
			 * its queue in case the newly scheduled thread is higher
			 * priority.
			 */
		}
	}

	return err;
}
