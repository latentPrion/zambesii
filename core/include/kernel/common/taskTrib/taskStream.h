#ifndef _TASK_STREAM_H
	#define _TASK_STREAM_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/slamCache.h>
	#include <__kclasses/prioQueue.h>
	#include <kernel/common/stream.h>
	#include <kernel/common/task.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>
	#include <kernel/common/taskTrib/prio.h>
	#include <kernel/common/taskTrib/load.h>

#define TASKSTREAM			"Task Stream "

#define TASK_SCHEDULE_TRY_AGAIN		0x1

class cpuStreamC;
class taskStreamC
:
public streamC
{
public:

	taskStreamC(cpuStreamC *parentCpu);

	/* Allocates internal queues, etc.
	 *	XXX:
	 * Take care not to allow this to be called twice on the BSP's task
	 * stream.
	 **/
	error_t initialize(void);

public:
	// Used by CPUs to get next task to execute.
	void pull(void);

	// cooperativeBind is only ever used at boot, on the BSP.
	error_t cooperativeBind(void);
	// error_t bind(void);
	// void cut(void);

	ubit32 getLoad(void) { return load; };
	ubit32 getCapacity(void) { return capacity; };
	void updateLoad(ubit8 action, ubit32 val);
	void updateCapacity(ubit8 action, ubit32 val);

	taskC *getCurrentTask(void) { return currentTask; }

	error_t schedule(taskC* task);

	void dormant(taskC *task)
	{
		task->runState = taskC::STOPPED;
		task->blockState = taskC::DORMANT;

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

	void block(taskC *task)
	{
		task->runState = taskC::STOPPED;
		task->blockState = taskC::BLOCKED;

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

	error_t unblock(taskC *task)
	{
		task->runState = taskC::RUNNABLE;

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

	void yield(taskC *task)
	{
		task->runState = taskC::RUNNABLE;

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

	error_t wake(taskC *task)
	{
		if (!(task->runState == taskC::STOPPED
			&& task->blockState == taskC::DORMANT))
		{
			return ERROR_SUCCESS;
		};

		task->runState = taskC::RUNNABLE;

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

	void dump(void);

private:
	taskC *pullRealTimeQ(void);
	taskC *pullRoundRobinQ(void);

public:
	ubit32		load;
	ubit32		capacity;
	taskC		*currentTask;

public:
	// Three queues on each CPU: rr, rt and sleep.
	prioQueueC<taskC>	roundRobinQ, realTimeQ;
	cpuStreamC		*parentCpu;
};

#endif

