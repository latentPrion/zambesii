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

#define SCHEDFLAGS_SCHED_WAITING	(1<<0)

#define TASKSTATE_DORMANT		0x1
#define TASKSTATE_RUNNABLE		0x2
#define TASKSTATE_RUNNING		0x3

class processStreamC;
class cpuStreamC;

struct taskS
{
	// Do *NOT* move 'stack' from where it is.
	void		*stack;

	// Basic information.
	uarch_t		id;
	processStreamC	*parent;
	taskContextS	*context;
	uarch_t		flags;

	// Scheduling information.
	prio_t		*schedPrio, internalPrio;
	ubit8		schedPolicy;
	ubit8		schedOptions;
	ubit8		schedFlags;
	ubit8		schedState;
	cpuStreamC	*currentCpu;

	// Miscellaneous properties (NUMA affinity, etc).
	ubit16		nLocksHeld;
	localAffinityS	localAffinity;
#ifdef CONFIG_PER_TASK_TLB_CONTEXT
	tlbContextS	*tlbContext;
#endif
};

#endif

