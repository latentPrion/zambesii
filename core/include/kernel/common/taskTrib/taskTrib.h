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

/**	Values for taskTribC::block()'s 'lockType' argument.
 **/
#define TASKTRIB_BLOCK_LOCKTYPE_WAIT			(0x0)
#define TASKTRIB_BLOCK_LOCKTYPE_RECURSIVE		(0x1)
#define TASKTRIB_BLOCK_LOCKTYPE_MULTIPLE_READER		(0x2)

/**	Values for taskTribC::block()'s 'unlockOp' argument. Only needed when
 * 'lockType' is MULTIPLE_READER.
 **/
#define TASKTRIB_BLOCK_UNLOCK_OP_READ			(0x0)
#define TASKTRIB_BLOCK_UNLOCK_OP_WRITE			(0x1)

class taskTribC
:
public tributaryC
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
	error_t schedule(threadC *thread);
	void yield(void);
	void block(void);
	// Used to prevent race conditions. See comments in definition.
	void block(
		lockC *lock, ubit8 lockType,
		ubit8 unlockOp=0, uarch_t unlockFlags=0);

	// Back ends.
	error_t dormant(taskC *task, taskContextC *perCpuContext=__KNULL);
	error_t wake(taskC *task, taskContextC *perCpuContext=__KNULL);
	error_t unblock(taskC *task, taskContextC *perCpuContext=__KNULL);

	/* These next few overloads are front-ends for the back ends that take
	 * pointers. The front-ends take IDs (either a CPU ID or a process ID)
	 * and quickly look up the ID and retrieve the correct pointer, before
	 * calling into the back end with the pointer.
	 **/
	error_t dormant(processId_t tid)
	{
		processStreamC	*proc;
		taskC		*task;

		proc = processTrib.getStream(tid);
		if (proc == __KNULL) { return ERROR_INVALID_ARG_VAL; };

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
		if (cpuStream == __KNULL) { return ERROR_INVALID_ARG_VAL; };

		return dormant(cpuStream);
	}

	error_t wake(processId_t tid)
	{
		processStreamC	*proc;
		taskC		*task;

		proc = processTrib.getStream(tid);
		if (proc == __KNULL) { return ERROR_INVALID_ARG_VAL; };

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
		if (cpuStream == __KNULL) { return ERROR_INVALID_ARG_VAL; };

		return wake(cpuStream);
	}

	error_t unblock(processId_t tid)
	{
		processStreamC	*proc;
		taskC		*task;

		proc = processTrib.getStream(tid);
		if (proc == __KNULL) { return ERROR_INVALID_ARG_VAL; };

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
		if (cpuStream == __KNULL) { return ERROR_INVALID_ARG_VAL; };

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
inline error_t taskTribC::schedule(taskC *task)
{
	return cpuTrib.getCurrentCpuStream()->taskStream.schedule(task);
}
#endif

#endif

