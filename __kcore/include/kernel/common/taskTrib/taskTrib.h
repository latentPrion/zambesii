#ifndef _TASK_TRIB_H
	#define _TASK_TRIB_H

	#include <scaling.h>
	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/thread.h>
	#include <kernel/common/lock.h>
	#include <kernel/common/taskTrib/prio.h>
	#include <kernel/common/taskTrib/taskQNode.h>
	#include <kernel/common/processTrib/processTrib.h>
	#include <kernel/common/cpuTrib/cpuTrib.h>
	#include <kernel/common/taskTrib/load.h>

#define TASKTRIB		"Task Trib: "

class TaskTrib
:
public Tributary
{
public:
	TaskTrib(void);
	error_t initialize(void) { return ERROR_SUCCESS; }

	void dump(void);

public:
	/* per-cpu threads are never submitted to the scheduler: they are
	 * spawned directly on their target CPU. If they are spawned DORMANT,
	 * when wake() is called on them, the target CPU's perCpuContext will be
	 * passed to wake().
	 **/
	error_t schedule(Thread *thread);
	void yield(void);
	// Used to prevent race conditions. See comments in definition.
	void block(Lock::sOperationDescriptor *unlockDescriptor=NULL);

	// Back ends.
	error_t dormant(Thread *thread);
	error_t wake(Thread *thread);
	error_t unblock(Thread *thread);
	void kill(Thread *thread) { dormant(thread); }

	/* These next few overloads are front-ends for the back ends that take
	 * pointers. The front-ends take IDs (either a CPU ID or a process ID)
	 * and quickly look up the ID and retrieve the correct pointer, before
	 * calling into the back end with the pointer.
	 **/
	error_t dormant(processId_t tid)
	{
		ProcessStream	*proc;
		Thread		*thread;

		proc = processTrib.getStream(tid);
		if (proc == NULL) { return ERROR_INVALID_ARG_VAL; };

		thread = proc->getThread(tid);
		return dormant(thread);
	}

	error_t wake(processId_t tid)
	{
		ProcessStream	*proc;
		Thread		*thread;

		proc = processTrib.getStream(tid);
		if (proc == NULL) { return ERROR_INVALID_ARG_VAL; };

		thread = proc->getThread(tid);
		return wake(thread);
	}

	error_t unblock(processId_t tid)
	{
		ProcessStream	*proc;
		Thread		*thread;

		proc = processTrib.getStream(tid);
		if (proc == NULL) { return ERROR_INVALID_ARG_VAL; };

		thread = proc->getThread(tid);
		return unblock(thread);
	}

	void kill(processId_t tid)
	{
		ProcessStream	*proc;
		Thread		*thread;

		proc = processTrib.getStream(tid);
		if (proc == NULL) { return; };

		thread = proc->getThread(tid);
		kill(thread);
	}

	ubit32 getLoad(void) { return load; };
	ubit32 getCapacity(void) { return capacity; };
	void updateLoad(ubit8 action, ubit32 val);
	void updateCapacity(ubit8 action, ubit32 val);

	// Create, alter and assign quantum classes.
	status_t createQuantumClass(utf8Char *name, prio_t softPrio);
	void setThreadQuantumClass(processId_t id, sarch_t qc);
	void setClassQuantum(sarch_t qc, prio_t softPrio);

private:
	SharedResourceGroup<WaitLock, sThreadQueueNode *>	deadQ;

	// Global machine scheduling statistics. Used for Oceann Zambesii.
	ubit32		load;
	ubit32		capacity;
};

extern TaskTrib	taskTrib;


/**	Inline methods:
 *****************************************************************************/

#if __SCALING__ == SCALING_UNIPROCESSOR
inline error_t TaskTrib::schedule(Thread *thread)
{
	return cpuTrib.getCurrentCpuStream()->taskStream.schedule(thread);
}
#endif

#endif

