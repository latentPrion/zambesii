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
	prio_t		prioClass[PRIO_NCLASSES];
	sharedResourceGroupC<waitLockC, taskListNodeS *>	deadQ;
};

#endif

