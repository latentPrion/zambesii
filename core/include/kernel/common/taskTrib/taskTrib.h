#ifndef _TASK_TRIB_H
	#define _TASK_TRIB_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/task.h>
	#include <kernel/common/taskTrib/prio.h>

class taskTribC
:
public tributaryC
{
public:
	taskTribC(void);

public:
	taskS *spawn(void);
	error_t destroy(void);

	taskS *getTask(processId_t id);

	// Create, alter and assign quantum classes.
	status_t createQuantumClass(utf16Char *name, prio_t softPrio);
	void setTaskQuantumClass(processId_t id, sarch_t qc);
	void setClassQuantum(sarch_t qc, prio_t softPrio);

private:
	prio_t		quantumClass[PRIOCLASS_NCLASSES];

	struct quantumClassS
	{
		prio_t		prio;
		utf16Char	name[64];
	};
	struct quantumClassStateS
	{
		quantumClassS	*arr;
		uarch_t		nClasses;
	};
	sharedResourceGroupC<waitLockC, quantumClassStateS> custQuantumClass;
	sharedResourceGroupC<waitLockC, taskListNodeS *>	deadQ;
};

#endif

