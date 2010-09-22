#ifndef _TASK_TRIB_H
	#define _TASK_TRIB_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/task.h>
	#include <kernel/common/taskTrib/prio.h>
	#include <kernel/common/taskTrib/taskQNode.h>
	#include <kernel/common/processTrib/processTrib.h>

class taskTribC
:
public tributaryC
{
public:
	taskTribC(void);

	void dump(void);

public:
	taskS *spawn(void);
	error_t destroy(void);

	taskS *getTask(processId_t id);

	// Create, alter and assign quantum classes.
	status_t createQuantumClass(utf16Char *name, prio_t softPrio);
	void setTaskQuantumClass(processId_t id, sarch_t qc);
	void setClassQuantum(sarch_t qc, prio_t softPrio);

private:
	prio_t		quantumClass[QUANTUMCLASS_NCLASSES];

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
	sharedResourceGroupC<waitLockC, taskQNodeS *>	deadQ;
};

extern taskTribC	taskTrib;


/**	Inline Methods.
 *****************************************************************************/

inline taskS *taskTribC::getTask(processId_t id)
{
	processS	*p;

	p = processTrib.getProcess(id);
	if (p == __KNULL) {
		return __KNULL;
	};

	for (taskS *tmp = p->head; tmp != __KNULL; tmp = tmp->next)
	{
		if (tmp->id == id) {
			return tmp;
		};
	};

	return __KNULL;
}

#endif

