
#include <scaling.h>
#include <__kstdlib/__kflagManipulation.h>
#include <kernel/common/taskTrib/taskStream.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


status_t taskStreamC::schedule(taskC *task)
{
	cpuStreamC	*curCpu;
	status_t	ret;

#if __SCALING__ >= SCALING_SMP
	curCpu = cpuTrib.getCurrentCpuStream();

	// Make sure that this CPU is in the task's affinity.
	if (!task->localAffinity.cpus.testSingle(curCpu->id)) {
		return TASK_SCHEDULE_TRY_AGAIN;
	};
#endif

	// Check CPU suitability to run task (FPU, other features).

	// Validate scheduling parameters.
	if (*task->schedPrio >= SCHEDPRIO_MAX_NPRIOS) {
		return ERROR_INVALID_ARG_VAL;
	};

	// Finally, add the task to a queue.
	switch (task->schedPolicy)
	{
	case SCHEDPOLICY_ROUND_ROBIN:
		task->schedState = TASKSTATE_RUNNABLE;
		ret = roundRobinQ.insert(
			task, *task->schedPrio, task->schedOptions);

		break;

	case SCHEDPOLICY_REAL_TIME:
		task->schedState = TASKSTATE_RUNNABLE;
		ret = realTimeQ.insert(
			task, *task->schedPrio, task->schedOptions);

		break;

	default:
		return ERROR_INVALID_ARG_VAL;
	};

	// Increment and notify upper layers of new task being scheduled.
	updateLoad(LOAD_UPDATE_ADD, 1);
	return ret;
}

void taskStreamC::pullTask(void)
{
	taskC		*newTask;
	cpuStreamC	*curCpu;

	curCpu = cpuTrib.getCurrentCpuStream();

	newTask = pullRealTimeQ();
	if (newTask != __KNULL) {
		goto execute;
	};

	newTask = pullRoundRobinQ();
	if (newTask != __KNULL) {
		goto execute;
	};

	// Else get an idle task from the main scheduler.

execute:
	newTask->currentCpu = curCpu;
	newTask->schedState = TASKSTATE_RUNNING;
	currentTask = newTask;

	// FIXME: Set nLocksHeld, etc here.

	// TODO: Pop task register context and jump.
}

taskC*taskStreamC::pullRealTimeQ(void)
{
	taskC		*ret;
	status_t	status;

	do
	{
		ret = static_cast<taskC*>( realTimeQ.pop() );
		if (ret == __KNULL) {
			return __KNULL;
		};

		// Make sure the scheduler isn't waiting for this task.
		if (__KFLAG_TEST(ret->schedFlags, SCHEDFLAGS_SCHED_WAITING))
		{
			status = taskStreamC::schedule(ret);
			continue;
		};

		return ret;
	} while (1);
}

taskC*taskStreamC::pullRoundRobinQ(void)
{
	taskC		*ret;
	status_t	status;
	cpuStreamC	*curCpu;

	curCpu = cpuTrib.getCurrentCpuStream();

	do
	{
		ret = static_cast<taskC*>( roundRobinQ.pop() );
		if (ret == __KNULL) {
			return __KNULL;
		};

		// Make sure the scheduler isn't waiting for this task.
		if (__KFLAG_TEST(ret->schedFlags, SCHEDFLAGS_SCHED_WAITING))
		{
			status = taskStreamC::schedule(ret);
			continue;
		};

		return ret;
	} while (1);
}

void taskStreamC::updateCapacity(ubit8 action, uarch_t val)
{
	numaCpuBankC		*ncb;

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
	if (ncb == __KNULL) { return; };

	ncb->updateCapacity(action, val);
}

void taskStreamC::updateLoad(ubit8 action, uarch_t val)
{
	numaCpuBankC		*ncb;

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
	if (ncb == __KNULL) { return; };

	ncb->updateLoad(action, val);
}

