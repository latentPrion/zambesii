
#include <scaling.h>
#include <arch/cpuControl.h>
#include <arch/taskContext.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/process.h>	
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

	return roundRobinQ.initialize();
}

void taskStreamC::dump(void)
{
	__kprintf(NOTICE TASKSTREAM"%d: load %d, capacity %d, "
		"currentTask 0x%x(@0x%p): dump.\n",
		parentCpu->id,
		load, capacity, currentTask->id, currentTask);

	realTimeQ.dump();
	roundRobinQ.dump();
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

/* Args:
 * __pb = pointer to the bitmapC object to be checked.
 * __n = the bit number which the bitmap should be able to hold. For example,
 *	 if the bit number is 32, then the BMP will be checked for 33 bit
 *	 capacity or higher, since 0-31 = 32 bits, and bit 32 would be the 33rd
 *	 bit.
 * __ret = pointer to variable to return the error code from the operation in.
 * __fn = The name of the function this macro was called inside.
 * __bn = the human recognizible name of the bitmapC instance being checked.
 *
 * The latter two are used to print out a more useful error message should an
 * error occur.
 **/
#define CHECK_AND_RESIZE_BMP(__pb,__n,__ret,__fn,__bn)			\
	do { \
	*(__ret) = ERROR_SUCCESS; \
	if ((__n) >= (signed)(__pb)->getNBits()) \
	{ \
		*(__ret) = (__pb)->resizeTo((__n) + 1); \
		if (*(__ret) != ERROR_SUCCESS) \
		{ \
			__kprintf(ERROR TASKSTREAM"%s: resize failed on %s " \
				"with required capacity = %d.\n", \
				__fn, __bn, __n); \
		}; \
	}; \
	} while (0);

status_t taskStreamC::schedule(taskC *task)
{
	status_t	ret;

#if __SCALING__ >= SCALING_SMP
	// Make sure that this CPU is in the task's affinity.
	if (!task->cpuAffinity.testSingle(parentCpu->id)) {
		return TASK_SCHEDULE_TRY_AGAIN;
	};

	CHECK_AND_RESIZE_BMP(
		&task->parent->cpuTrace, parentCpu->id, &ret,
		"schedule", "cpuTrace");

	if (ret != ERROR_SUCCESS) { return ret; };
#endif
	// Check CPU suitability to run task (FPU, other features).

	// Validate any scheduling parameters that need to be checked.

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

	task->parent->cpuTrace.setSingle(parentCpu->id);
	// Increment and notify upper layers of new task being scheduled.
	updateLoad(LOAD_UPDATE_ADD, 1);
	return ret;
}

void taskStreamC::pull(void)
{
	taskC		*newTask;

	for (;;)
	{
		newTask = pullRealTimeQ();
		if (newTask != __KNULL) {
			break;
		};

		newTask = pullRoundRobinQ();
		if (newTask != __KNULL) {
			break;
		};

		// Else set the CPU to a low power state.
		cpuControl::halt();
	};

	newTask->runState = taskC::RUNNING;
	currentTask = newTask;			
	loadContextAndJump(newTask->context);
}

taskC* taskStreamC::pullRealTimeQ(void)
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
			status = schedule(ret);
			continue;
		};

		return ret;
	} while (1);
}

taskC* taskStreamC::pullRoundRobinQ(void)
{
	taskC		*ret;
	status_t	status;

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
			status = schedule(ret);
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

