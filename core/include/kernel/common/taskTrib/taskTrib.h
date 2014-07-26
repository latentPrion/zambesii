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

class taskTribC
:
public Tributary
{
public:
	taskTribC(void);

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
	void block(Lock::operationDescriptorS *unlockDescriptor=NULL);

	// Back ends.
	error_t dormant(Task *task, TaskContext *perCpuContext=NULL);
	error_t wake(Task *task, TaskContext *perCpuContext=NULL);
	error_t unblock(Task *task, TaskContext *perCpuContext=NULL);

	/* These next few overloads are front-ends for the back ends that take
	 * pointers. The front-ends take IDs (either a CPU ID or a process ID)
	 * and quickly look up the ID and retrieve the correct pointer, before
	 * calling into the back end with the pointer.
	 **/
	error_t dormant(processId_t tid)
	{
		processStreamC	*proc;
		Task		*task;

		proc = processTrib.getStream(tid);
		if (proc == NULL) { return ERROR_INVALID_ARG_VAL; };

		task = proc->getTask(tid);
		return dormant(task);
	}

	error_t dormant(cpuStreamC *cpuStream)
	{
		return dormant(
			cpuStream->taskStream.getCurrentPerCpuTask(),
			cpuStream->getTaskContext());
	}

	error_t dormant(cpu_t cid)
	{
		cpuStreamC	*cpuStream;

		cpuStream = cpuTrib.getStream(cid);
		if (cpuStream == NULL) { return ERROR_INVALID_ARG_VAL; };

		return dormant(cpuStream);
	}

	error_t wake(processId_t tid)
	{
		processStreamC	*proc;
		Task		*task;

		proc = processTrib.getStream(tid);
		if (proc == NULL) { return ERROR_INVALID_ARG_VAL; };

		task = proc->getTask(tid);
		return wake(task);
	}

	error_t wake(cpuStreamC *cpuStream)
	{
		return wake(
			cpuStream->taskStream.getCurrentPerCpuTask(),
			cpuStream->getTaskContext());
	}

	error_t wake(cpu_t cid)
	{
		cpuStreamC	*cpuStream;

		cpuStream = cpuTrib.getStream(cid);
		if (cpuStream == NULL) { return ERROR_INVALID_ARG_VAL; };

		return wake(cpuStream);
	}

	error_t unblock(processId_t tid)
	{
		processStreamC	*proc;
		Task		*task;

		proc = processTrib.getStream(tid);
		if (proc == NULL) { return ERROR_INVALID_ARG_VAL; };

		task = proc->getTask(tid);
		return unblock(task);
	}

	error_t unblock(cpuStreamC *cpuStream)
	{
		return unblock(
			cpuStream->taskStream.getCurrentPerCpuTask(),
			cpuStream->getTaskContext());
	}

	error_t unblock(cpu_t cid)
	{
		cpuStreamC	*cpuStream;

		cpuStream = cpuTrib.getStream(cid);
		if (cpuStream == NULL) { return ERROR_INVALID_ARG_VAL; };

		return unblock(cpuStream);
	}

	ubit32 getLoad(void) { return load; };
	ubit32 getCapacity(void) { return capacity; };
	void updateLoad(ubit8 action, ubit32 val);
	void updateCapacity(ubit8 action, ubit32 val);

	// Create, alter and assign quantum classes.
	status_t createQuantumClass(utf8Char *name, prio_t softPrio);
	void setTaskQuantumClass(processId_t id, sarch_t qc);
	void setClassQuantum(sarch_t qc, prio_t softPrio);

private:
	sharedResourceGroupC<waitLockC, taskQNodeS *>	deadQ;

	// Global machine scheduling statistics. Used for Oceann Zambesii.
	ubit32		load;
	ubit32		capacity;
};

extern taskTribC	taskTrib;


/**	Inline methods:
 *****************************************************************************/

#if __SCALING__ == SCALING_UNIPROCESSOR
inline error_t taskTribC::schedule(Task *task)
{
	return cpuTrib.getCurrentCpuStream()->taskStream.schedule(task);
}
#endif

#endif

