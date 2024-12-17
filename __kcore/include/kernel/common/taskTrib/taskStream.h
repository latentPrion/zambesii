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
: public Stream<CpuStream>
{
friend class CpuStream;
public:
	TaskStream(cpu_t cid, CpuStream *parentCpu);

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
	//virtual error_t bind(void);
	//virtual void cut(void);

	ubit32 getLoad(void) { return load; };
	ubit32 getCapacity(void) { return capacity; };
	void updateLoad(ubit8 action, ubit32 val);
	void updateCapacity(ubit8 action, ubit32 val);

	Thread *getCurrentThread(void) { return currentThread; }

	error_t schedule(Thread *thread);

	void yield(Thread *thread);
	error_t wake(Thread *thread);
	void dormant(Thread *thread);
	void block(Thread *thread);
	error_t unblock(Thread *thread);
	/**	Unimplemented.
	 * void kill(Thread *thread);
	 **/

	void dump(void);

private:
	Thread *pullFromQ(utf8Char *queueName);

public:
	ubit32		load;
	ubit32		capacity;

private:
	Thread		*currentThread;

public:
	// Three queues on each CPU: rr, rt and sleep.
	PrioQueue<Thread>	roundRobinQ, realTimeQ;
	Thread			powerThread;
	// Stack for the CPU's power thread.
	ubit8			powerStack[
		PAGING_BASE_SIZE * CHIPSET_MEMORY___KSTACK_NPAGES];
};


#endif

