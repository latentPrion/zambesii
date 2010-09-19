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

	

private:
	prio_t		prioClass[PRIOCLASS_NCLASSES];
	prio_t		*customPrioClass;
	sharedResourceGroupC<waitLockC, taskListNodeS *>	deadQ;
};

#endif

