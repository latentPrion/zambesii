
#include <scaling.h>
#include <kernel/common/taskTrib/taskStream.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


status_t taskStreamC::schedule(taskS *task)
{
	cpuStreamC	*curCpu;
	status_t	ret;

#if __SCALING__ >= SCALING_SMP
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
		ret = roundRobinQ.insert(
			task, task->schedPrio, task->schedFlags);

		break;

	case SCHEDPOLICY_REAL_TIME:
		ret = realTimeQ.insert(
			task, task->schedPrio, task->schedFlags);

		break;

	default:
		return ERROR_INVALID_ARG_VAL;
	};

	return ret;
}



void taskTribC::updateCapacity(ubit8 action, uarch_t val)
{
	switch (action)
	{
	case PROCESSTRIB_UPDATE_ADD:
		capacity += val;
		return;

	case PROCESSTRIB_UPDATE_SUBTRACT:
		capacity -= val;
		return;

	case PROCESSTRIB_UPDATE_SET:
		capacity = val;
		return;

	default: return;
	};
}

void taskTribC::updateLoad(ubit8 action, uarch_t val)
{
	switch (action)
	{
	case PROCESSTRIB_UPDATE_ADD:
		load += val;
		return;

	case PROCESSTRIB_UPDATE_SUBTRACT:
		load -= val;
		return;

	case PROCESSTRIB_UPDATE_SET:
		load = val;
		return;

	default: return;
	};
}

