
#include <scaling.h>
#include <arch/cpuControl.h>
#include <arch/taskContext.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/taskTrib/taskStream.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <__kthreads/__kcpuPowerOn.h>
#include <__kthreads/__korientation.h>


extern "C" void taskStream_pull(taskContextC *savedContext)
{
	cpuStreamC	*currCpu;

	// XXX: We are operating on the CPU's sleep stack.
	currCpu = cpuTrib.getCurrentCpuStream();
	if (savedContext != __KNULL) {
		currCpu->taskStream.currentTask->context = savedContext;
	};

	currCpu->taskStream.pull();
}

taskStreamC::taskStreamC(cpuStreamC *parent)
:
load(0), capacity(0),
roundRobinQ(SCHEDPRIO_MAX_NPRIOS), realTimeQ(SCHEDPRIO_MAX_NPRIOS),
parentCpu(parent)
{
	/* Ensure that the BSP CPU's pointer to __korientation isn't trampled
	 * by the general case constructor.
	 **/
	if (!__KFLAG_TEST(parentCpu->flags, CPUSTREAM_FLAGS_BSP)) {
		currentTask = &__kcpuPowerOnThread;
	};
}

error_t taskStreamC::initialize(void)
{
	error_t		ret;

	ret = realTimeQ.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	ret = roundRobinQ.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	//ret = dormantQ.initialize();
	//if (ret != ERROR_SUCCESS) { return ret; };

	return ERROR_SUCCESS;
}

void taskStreamC::dump(void)
{
	__kprintf(NOTICE TASKSTREAM"%d: load %d, capacity %d, "
		"currentTask 0x%x(@0x%p): dump.\n",
		parentCpu->id,
		load, capacity, currentTask->id, currentTask);

	roundRobinQ.dump();
	realTimeQ.dump();
}

error_t taskStreamC::cooperativeBind(void)
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
	/*__korientationThread.schedPolicy = taskC::ROUND_ROBIN;
	__korientationThread.schedOptions = 0;
	__korientationThread.schedFlags = 0;

	return roundRobinQ.insert(
		&__korientationThread,
		__korientationThread.schedPrio->prio,
		__korientationThread.schedOptions);*/

	cpuTrib.onlineCpus.setSingle(parentCpu->id);
	return ERROR_SUCCESS;
}

status_t taskStreamC::schedule(taskC *task)
{
	status_t	ret;

#if __SCALING__ >= SCALING_SMP
	// Make sure that this CPU is in the task's affinity.
	if (!task->cpuAffinity.testSingle(parentCpu->id)) {
		return TASK_SCHEDULE_TRY_AGAIN;
	};
#endif
	// Check CPU suitability to run task (FPU, other features).

	// Validate scheduling parameters.
	if (task->schedPrio->prio >= SCHEDPRIO_MAX_NPRIOS) {
		return ERROR_UNSUPPORTED;
	};

	task->runState = taskC::RUNNABLE;
	task->currentCpu = parentCpu;

	// Finally, add the task to a queue.
	switch (task->schedPolicy)
	{
	case taskC::ROUND_ROBIN:
		ret = roundRobinQ.insert(
			task, task->schedPrio->prio, task->schedOptions);

		break;

	case taskC::REAL_TIME:
		ret = realTimeQ.insert(
			task, task->schedPrio->prio, task->schedOptions);

		break;

	default:
		return ERROR_INVALID_ARG_VAL;
	};

	if (ret != ERROR_SUCCESS) { return ret; };
	// Increment and notify upper layers of new task being scheduled.
	updateLoad(LOAD_UPDATE_ADD, 1);
	return ret;
}

void taskStreamC::pull(void)
{
	taskC		*newTask;

	newTask = pullRealTimeQ();
	if (newTask != __KNULL) {
		goto execute;
	};

	newTask = pullRoundRobinQ();
	if (newTask != __KNULL) {
		goto execute;
	};

	// Else set the CPU to a low power state.
	__kprintf(NOTICE TASKSTREAM"%d: Entering C1.\n", parentCpu->id);
	for (;;) {
		cpuControl::halt();
	};

execute:
	newTask->runState = taskC::RUNNING;
	currentTask = newTask;
	// Jump to the newly pulled task.
	loadContextAndJump(newTask->context);
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
		if (__KFLAG_TEST(
			ret->schedFlags, TASK_SCHEDFLAGS_SCHED_WAITING))
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
		if (__KFLAG_TEST(
			ret->schedFlags, TASK_SCHEDFLAGS_SCHED_WAITING))
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

