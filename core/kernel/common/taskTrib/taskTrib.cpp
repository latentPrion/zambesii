
#include <scaling.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/numaCpuBank.h>
#include <kernel/common/panic.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


taskTribC::taskTribC(void)
{
	capacity = 0;
	load = 0;
}

#if __SCALING__ >= SCALING_SMP
error_t taskTribC::schedule(taskC *task)
{
	cpuStreamC	*cs, *bestCandidate=__KNULL;

	for (cpu_t i=0; i<(signed)task->cpuAffinity.getNBits(); i++)
	{
		if (task->cpuAffinity.testSingle(i)
			&& cpuTrib.onlineCpus.testSingle(i))
		{
			cs = cpuTrib.getStream(i);

			if (bestCandidate == __KNULL)
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

	if (bestCandidate == __KNULL) { return ERROR_UNKNOWN; };
	return bestCandidate->taskStream.schedule(task);
}
#endif

void taskTribC::updateCapacity(ubit8 action, uarch_t val)
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

void taskTribC::updateLoad(ubit8 action, uarch_t val)
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

error_t taskTribC::dormant(taskC *task)
{
	cpuStreamC	*currentCpu;

	if (task->runState == taskC::UNSCHEDULED
		|| (task->runState == taskC::STOPPED
			&& task->blockState == taskC::DORMANT))
	{
		return ERROR_SUCCESS;
	};

	task->currentCpu->taskStream.dormant(task);

	currentCpu = cpuTrib.getCurrentCpuStream();
	if (task->currentCpu != currentCpu)
	{
		/* Message the CPU to pre-empt and choose another thread.
		 * Should probably take an argument for the thread to be
		 * pre-empted.
		 **/
	}
	else
	{
		if (task == currentCpu->taskStream.getCurrentTask())
		{
			saveContextAndCallPull(
				&currentCpu->sleepStack[
					sizeof(currentCpu->sleepStack)]);

			// Unreachable.
		};
	};

	return ERROR_SUCCESS;
}

error_t taskTribC::wake(taskC *task)
{
	if (task->runState != taskC::UNSCHEDULED
		&& (task->runState != taskC::STOPPED
			&& task->blockState != taskC::DORMANT))
	{
		return ERROR_SUCCESS;
	};

	if (task->runState == taskC::UNSCHEDULED) {
		return schedule(task);
	} else {
		return task->currentCpu->taskStream.wake(task);
	};
}

void taskTribC::yield(void)
{
	taskC		*currTask;

	currTask = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask();
	currTask->currentCpu->taskStream.yield(currTask);

	saveContextAndCallPull(
		&currTask->currentCpu->sleepStack[
			sizeof(currTask->currentCpu->sleepStack)]);
}

void taskTribC::block(void)
{
	taskC		*currTask;

	/**	XXX:
	 * If you update this function, be sure to update its overloaded version
	 * below as well.
	 *
	 *	NOTES:
	 * After placing a task into a waitqueue, call this function to
	 * place it into a "blocked" state.
	 **/
	currTask = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask();
	currTask->currentCpu->taskStream.block(currTask);

	saveContextAndCallPull(
		&currTask->currentCpu->sleepStack[
			sizeof(currTask->currentCpu->sleepStack)]);
}

void taskTribC::block(
	lockC *lock, ubit8 lockType, ubit8 unlockOp, uarch_t unlockFlags
	)
{
	taskC		*currTask;

	/**	XXX:
	 * If you update this function, be sure to update its overloaded version
	 * above as well.
	 *
	 *	NOTES:
	 * After placing a task into a waitqueue, call this function to
	 * place it into a "blocked" state.
	 **/
	if (lock == __KNULL) { panic(FATAL"block(): 'lock' is __KNULL.\n"); };

	currTask = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask();
	currTask->currentCpu->taskStream.block(currTask);

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
	switch (lockType)
	{
	case TASKTRIB_BLOCK_LOCKTYPE_WAIT:
		static_cast<waitLockC *>( lock )->release();
		break;

	case TASKTRIB_BLOCK_LOCKTYPE_RECURSIVE:
		static_cast<recursiveLockC *>( lock )->release();
		break;

	case TASKTRIB_BLOCK_LOCKTYPE_MULTIPLE_READER:
		switch (unlockOp)
		{
		case TASKTRIB_BLOCK_UNLOCK_OP_READ:
			static_cast<multipleReaderLockC *>( lock )
				->readRelease(unlockFlags);

			break;

		case TASKTRIB_BLOCK_UNLOCK_OP_WRITE:
			static_cast<multipleReaderLockC *>( lock )
				->writeRelease();

			break;

		default:
			panic(FATAL"block(): Invalid unlock operation.\n");
		};
		break;

	default: panic(FATAL"block(): Invalid lockType.\n");
	};

	saveContextAndCallPull(
		&currTask->currentCpu->sleepStack[
			sizeof(currTask->currentCpu->sleepStack)]);
}

static utf8Char		*runStates[5] =
{
	CC"invalid", CC"unscheduled", CC"runnable", CC"running", CC"stopped"
};

error_t taskTribC::unblock(taskC *task)
{
	if (task->runState == taskC::RUNNABLE
		|| task->runState == taskC::RUNNING)
	{
		return ERROR_SUCCESS;
	};

	if (!(task->runState == taskC::STOPPED
		&& task->blockState == taskC::BLOCKED))
	{
		__kprintf(NOTICE TASKTRIB"unblock(0x%x): Invalid run state."
			"runState is %s.\n",
			task->getFullId(),
			runStates[task->runState]);

		return ERROR_INVALID_OPERATION;
	};

	return task->currentCpu->taskStream.unblock(task);
}

