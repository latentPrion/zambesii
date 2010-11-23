#ifndef _TASK_H
	#define _TASK_H

	#include <arch/taskContext.h>
	#include <arch/tlbContext.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/bitmap.h>
	#include <kernel/common/machineAffinity.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/multipleReaderLock.h>
	#include <kernel/common/process.h>
	#include <kernel/common/taskTrib/prio.h>

#define TASK_FLAGS_SCHED_WAITING	(1<<0)

struct processStreamC;

struct taskS
{
	// Do *NOT* move 'stack' from where it is.
	void		*stack;
	uarch_t		id;
	taskS		*next;
	taskContextS	*context;
	uarch_t		flags;
	prio_t		*prio, internalPrio;
	ubit16		nLocksHeld;
	processStreamC	*parent;
	localAffinityS	localAffinity;
#ifdef CONFIG_PER_TASK_TLB_CONTEXT
	tlbContextS	*tlbContext;
#endif
};

namespace task
{
	error_t initialize(taskS *task);
	void destroy(taskS *task);
}

#endif

