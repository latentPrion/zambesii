#ifndef _TASK_STREAM_H
	#define _TASK_STREAM_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/slamCache.h>
	#include <__kclasses/prioQueue.h>
	#include <kernel/common/stream.h>
	#include <kernel/common/thread.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>
	#include <kernel/common/taskTrib/prio.h>
	#include <kernel/common/taskTrib/load.h>

#define TASKSTREAM			"Task Stream "

#define TASK_SCHEDULE_TRY_AGAIN		0x1

class CpuStream;
class TaskStream
:
public Stream
{
friend class CpuStream;
public:

	TaskStream(CpuStream *parentCpu);

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

	Task *getCurrentTask(void) { return currentTask; }
	PerCpuThread *getCurrentPerCpuTask(void) { return currentPerCpuThread; }
	TaskContext *getCurrentTaskContext(void);
	processId_t getCurrentTaskId(void);

	error_t schedule(Task* task);

	void yield(Task *task);
	error_t wake(Task *task);
	void dormant(Task *task);
	void block(Task *task);
	error_t unblock(Task *task);

	void dump(void);

private:
	Task *pullRealTimeQ(void);
	Task *pullRoundRobinQ(void);

public:
	ubit32		load;
	ubit32		capacity;

private:
	Task		*currentTask;
	PerCpuThread	*currentPerCpuThread;

public:
	// Three queues on each CPU: rr, rt and sleep.
	PrioQueue<Task>	roundRobinQ, realTimeQ;
	CpuStream		*parentCpu;
};

#endif

