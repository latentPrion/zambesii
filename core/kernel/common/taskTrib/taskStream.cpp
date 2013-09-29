
#include <scaling.h>
#include <arch/cpuControl.h>
#include <arch/registerContext.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/process.h>	
#include <kernel/common/taskTrib/taskStream.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <__kthreads/__kcpuPowerOn.h>
#include <__kthreads/__korientation.h>


extern "C" void taskStream_pull(registerContextC *savedContext)
{
	taskStreamC	*currTaskStream;

	// XXX: We are operating on the CPU's sleep stack.
	currTaskStream = &cpuTrib.getCurrentCpuStream()->taskStream;
	if (savedContext != NULL) {
		currTaskStream->getCurrentTaskContext()->context = savedContext;
	};

	currTaskStream->pull();
}

taskStreamC::taskStreamC(cpuStreamC *parent)
:
streamC(0),
load(0), capacity(0),
currentPerCpuThread(NULL),
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
		parentCpu->cpuId,
		load, capacity,
		(getCurrentTask()->getType() == task::PER_CPU)
			? ((threadC *)getCurrentTask())->getFullId()
			: parentCpu->cpuId,
		getCurrentTask());

	realTimeQ.dump();
	roundRobinQ.dump();
}

taskContextC *taskStreamC::getCurrentTaskContext(void)
{
	if (getCurrentTask()->getType() == task::PER_CPU) {
		return parentCpu->getTaskContext();
	} else {
		return ((threadC *)getCurrentTask())->getTaskContext();
	};
}

processId_t taskStreamC::getCurrentTaskId(void)
{
	if (getCurrentTask()->getType() == task::PER_CPU) {
		return (processId_t)parentCpu->cpuId;
	} else {
		return ((threadC *)getCurrentTask())->getFullId();
	};
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

	cpuTrib.onlineCpus.setSingle(parentCpu->cpuId);
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
	taskContextC	*taskContext;

	/**	EXPLANATION:
	 * TaskStreamC::schedule() is the lowest level schedule() call; it is
	 * per-CPU in nature and it is the only schedule() layer that can
	 * accept a per-cpu thread as an argument since per-cpu threads are
	 * directly scheduled to their target CPUs (using this function).
	 *
	 * For a normal thread, we just schedule it normally. For per-cpu
	 * threads, we also have to set the CPU's currentPerCpuThread pointer.
	 **/
	taskContext = task->getType() == (task::PER_CPU)
		? parentCpu->getTaskContext()
		: static_cast<threadC *>( task )->getTaskContext();

#if __SCALING__ >= SCALING_SMP
	// We don't need to test this for per-cpu threads.
	if (task->getType() == task::UNIQUE)
	{
		// Make sure that this CPU is in the thread's affinity.
		if (!taskContext->cpuAffinity.testSingle(parentCpu->cpuId))
			{ return TASK_SCHEDULE_TRY_AGAIN; };
	};

	CHECK_AND_RESIZE_BMP(
		&task->parent->cpuTrace, parentCpu->cpuId, &ret,
		"schedule", "cpuTrace");

	if (ret != ERROR_SUCCESS) { return ret; };
#endif
	// Check CPU suitability to run task (FPU, other features).

	// Validate any scheduling parameters that need to be checked.

	taskContext->runState = taskContextC::RUNNABLE;
	if (task->getType() == task::UNIQUE)
		{ ((threadC *)task)->currentCpu = parentCpu; }
	else
		{ currentPerCpuThread = (perCpuThreadC *)task; };

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

	task->parent->cpuTrace.setSingle(parentCpu->cpuId);
	// Increment and notify upper layers of new task being scheduled.
	updateLoad(LOAD_UPDATE_ADD, 1);
	return ret;
}

static inline void getCr3(paddr_t *ret)
{
	asm volatile("movl	%%cr3, %0\n\t"
		: "=r" (*ret));
}

void taskStreamC::pull(void)
{
	taskC		*newTask;

	for (;;)
	{
		newTask = pullRealTimeQ();
		if (newTask != NULL) {
			break;
		};

		newTask = pullRoundRobinQ();
		if (newTask != NULL) {
			break;
		};

		// Else set the CPU to a low power state.
		if (/*!__KFLAG_TEST(parentCpu->flags, CPUSTREAM_FLAGS_BSP)*/ 0)
		{
			__kprintf(NOTICE TASKSTREAM"%d: Entering C1.\n",
				parentCpu->cpuId);
		};

		cpuControl::halt();
	};

	// FIXME: Should take a tlbContextC object instead.
	tlbControl::saveContext(
		&currentTask->parent->getVaddrSpaceStream()->vaddrSpace);

	// If addrspaces differ, switch; don't switch if kernel/per-cpu thread.
	if (newTask->parent->getVaddrSpaceStream()
		!= currentTask->parent->getVaddrSpaceStream()
		&& newTask->parent->id != __KPROCESSID)
	{
		tlbControl::loadContext(
			&newTask->parent->getVaddrSpaceStream()
				->vaddrSpace);
	};

	taskContextC		*newTaskContext;

	newTaskContext = (newTask->getType() == task::PER_CPU)
		? parentCpu->getTaskContext()
		: static_cast<threadC *>( newTask )->getTaskContext();

	newTaskContext->runState = taskContextC::RUNNING;
	currentTask = newTask;
	loadContextAndJump(newTaskContext->context);
}

// TODO: Merge the two queue pull functions for cache efficiency.
taskC* taskStreamC::pullRealTimeQ(void)
{
	taskC		*ret;
	status_t	status;

	do
	{
		ret = static_cast<taskC*>( realTimeQ.pop() );
		if (ret == NULL) {
			return NULL;
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
		if (ret == NULL) {
			return NULL;
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
	if (ncb == NULL) { return; };

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
	if (ncb == NULL) { return; };

	ncb->updateLoad(action, val);
}

void taskStreamC::dormant(taskC *task)
{
	taskContextC		*taskContext;

	taskContext = (task->getType() == task::PER_CPU)
		? parentCpu->getTaskContext()
		: static_cast<threadC *>( task )->getTaskContext();

	taskContext->runState = taskContextC::STOPPED;
	taskContext->blockState = taskContextC::DORMANT;

	switch (task->schedPolicy)
	{
	case taskC::ROUND_ROBIN:
		roundRobinQ.remove(task, task->schedPrio->prio);
		break;

	case taskC::REAL_TIME:
		realTimeQ.remove(task, task->schedPrio->prio);
		break;

	default: // Might want to panic or make some noise here.
		return;
	};
}

void taskStreamC::block(taskC *task)
{
	taskContextC		*taskContext;

	taskContext = (task->getType() == task::PER_CPU)
		? parentCpu->getTaskContext()
		: static_cast<threadC *>( task )->getTaskContext();

	taskContext->runState = taskContextC::STOPPED;
	taskContext->blockState = taskContextC::BLOCKED;

	switch (task->schedPolicy)
	{
	case taskC::ROUND_ROBIN:
		roundRobinQ.remove(task, task->schedPrio->prio);
		break;

	case taskC::REAL_TIME:
		realTimeQ.remove(task, task->schedPrio->prio);
		break;

	default:
		return;
	};
}

error_t taskStreamC::unblock(taskC *task)
{
	taskContextC		*taskContext;

	taskContext = (task->getType() == task::PER_CPU)
		? parentCpu->getTaskContext()
		: static_cast<threadC *>( task )->getTaskContext();

	taskContext->runState = taskContextC::RUNNABLE;

	switch (task->schedPolicy)
	{
	case taskC::ROUND_ROBIN:
		return roundRobinQ.insert(
			task, task->schedPrio->prio,
			task->schedOptions);

	case taskC::REAL_TIME:
		return realTimeQ.insert(
			task, task->schedPrio->prio,
			task->schedOptions);

	default:
		return ERROR_UNSUPPORTED;
	};
}

void taskStreamC::yield(taskC *task)
{
	taskContextC		*taskContext;

	taskContext = (task->getType() == task::PER_CPU)
		? parentCpu->getTaskContext()
		: static_cast<threadC *>( task )->getTaskContext();

	taskContext->runState = taskContextC::RUNNABLE;

	switch (task->schedPolicy)
	{
	case taskC::ROUND_ROBIN:
		roundRobinQ.insert(
			task, task->schedPrio->prio,
			task->schedOptions);
		break;

	case taskC::REAL_TIME:
		realTimeQ.insert(
			task, task->schedPrio->prio,
			task->schedOptions);
		break;

	default:
		return;
	};
}

error_t taskStreamC::wake(taskC *task)
{
	taskContextC		*taskContext;

	taskContext = (task->getType() == task::PER_CPU)
		? parentCpu->getTaskContext()
		: static_cast<threadC *>( task )->getTaskContext();

	if (!(taskContext->runState == taskContextC::STOPPED
		&& taskContext->blockState == taskContextC::DORMANT)
		&& taskContext->runState != taskContextC::UNSCHEDULED)
	{
		return ERROR_UNSUPPORTED;
	};

	taskContext->runState = taskContextC::RUNNABLE;

	switch (task->schedPolicy)
	{
	case taskC::ROUND_ROBIN:
		return roundRobinQ.insert(
			task, task->schedPrio->prio,
			task->schedOptions);

	case taskC::REAL_TIME:
		return realTimeQ.insert(
			task, task->schedPrio->prio,
			task->schedOptions);

	default:
		return ERROR_CRITICAL;
	};
}

