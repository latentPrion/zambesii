#ifndef _TASK_TRIB_H
	#define _TASK_TRIB_H

	#include <scaling.h>
	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/task.h>
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
	error_t schedule(taskC *task);
	void yield(void);
	void block(void);
	// Used to prevent race conditions. See comments in definition.
	void block(
		lockC *lock, ubit8 lockType,
		ubit8 unlockOp=0, uarch_t unlockFlags=0);

	error_t dormant(taskC *task);
	error_t dormant(processId_t tid)
	{
		processStreamC	*proc;
		taskC		*task;

		proc = processTrib.getStream(tid);
		if (proc == __KNULL) { return ERROR_INVALID_ARG_VAL; };

		task = proc->getTask(tid);
		return dormant(task);
	}

	error_t wake(taskC *task);
	error_t wake(processId_t tid)
	{
		processStreamC	*proc;
		taskC		*task;

		proc = processTrib.getStream(tid);
		if (proc == __KNULL) { return ERROR_INVALID_ARG_VAL; };

		task = proc->getTask(tid);
		return wake(task);
	}

	error_t unblock(taskC *task);
	error_t unblock(processId_t tid)
	{
		processStreamC	*proc;
		taskC		*task;

		proc = processTrib.getStream(tid);
		if (proc == __KNULL) { return ERROR_INVALID_ARG_VAL; };

		task = proc->getTask(tid);
		return unblock(task);
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

	// Global machine scheduling statistics. Used for Ocean Zambesii.
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

