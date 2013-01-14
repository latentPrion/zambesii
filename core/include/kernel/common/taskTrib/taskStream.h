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
friend class cpuStreamC;
public:
	taskStreamC(cpuStreamC *parent);
	/* Allocates internal queues, etc.
	 *	XXX:
	 * Take care not to allow this to be called twice on the BSP's task
	 * stream.
	 **/
	error_t initialize(void);

public:
	// cooperativeBind is only ever used at boot, on the BSP.
	error_t cooperativeBind(void);
	// error_t bind(void);
	// void cut(void);

	ubit32 getLoad(void) { return load; };
	ubit32 getCapacity(void) { return capacity; };
	void updateLoad(ubit8 action, ubit32 val);
	void updateCapacity(ubit8 action, ubit32 val);

	error_t schedule(taskC* task);

	void dump(void);

private:
	// Used by CPU to get next task to execute.
	void pullTask(void);

	taskC *pullRealTimeQ(void);
	taskC *pullRoundRobinQ(void);

public:
	ubit32		load;
	ubit32		capacity;
	taskC		*currentTask;

private:
	// Three queues on each CPU: rr, rt and sleep.
	prioQueueC<taskC>	roundRobinQ, realTimeQ, dormantQ;
	cpuStreamC		*parentCpu;
};

#endif

