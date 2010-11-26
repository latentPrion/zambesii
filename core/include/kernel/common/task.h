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

/**	EXPLANATION:
 * To spawn a new thread, first the caller must ask the thread's parent if there
 * are any remaining slots in the process's thread list. If there are none,
 * too bad.
 *
 * Assuming that the parent process was able to justify a new thread, it will
 * consume a new thread ID, and fill a new slot in its thread list, and assign
 * the new thread that thread ID. The parent process also fills out the new
 * thread's default values for affinity, and scheduling parameters. The new
 * thread is then returned to the caller.
 *
 * Next, the caller must call initialize() on the new thread so that the
 * thread's register context, ring0 & 3 stacks, and possible TLB context can be
 * allocated.
 *
 * After that, the caller must call initialize() on the new thread's register
 * context and TLB context. This is to ensure that both structures are populated
 * with acceptable default values. At this point, the new thread is ready to be
 * executed.
 *
 * The caller now calls the Task Tributary's schedule() method, which takes the
 * new thread and adds it to one of its bound CPUs' queues. The new thread has
 * now been fully spawned and scheduled.
 **/

#define SCHEDFLAGS_SCHED_WAITING	(1<<0)

#define TASKSTATE_DORMANT		0x1
#define TASKSTATE_RUNNABLE		0x2
#define TASKSTATE_RUNNING		0x3

class processStreamC;
class cpuStreamC;

class taskC
{
public:
	// Do *NOT* move 'stack' from where it is.
	void		*stack0, *stack1;

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

