
#include <scaling.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/numaCpuBank.h>
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

	/* After placing a task into a waitqueue, call this function to
	 * place it into a "blocked" state.
	 **/
	currTask = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask();
	currTask->currentCpu->taskStream.block(currTask);

	saveContextAndCallPull(
		&currTask->currentCpu->sleepStack[
			sizeof(currTask->currentCpu->sleepStack)]);
}

error_t taskTribC::unblock(taskC *task)
{
	if (task->runState == taskC::RUNNABLE)
	{
		__kprintf(NOTICE TASKTRIB"unblock(0x%x): already runnable.\n",
			task->id);

		return ERROR_SUCCESS;
	};

	if (!(task->runState == taskC::STOPPED
		&& task->blockState == taskC::BLOCKED))
	{
		__kprintf(NOTICE TASKTRIB"unblock(0x%x): Invalid run state.\n",
			task->id);

		return ERROR_INVALID_OPERATION;
	};

	return task->currentCpu->taskStream.unblock(task);
}

