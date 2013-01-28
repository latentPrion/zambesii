#ifndef _TASK_H
	#define _TASK_H

	#include <scaling.h>
	#include <arch/taskContext.h>
	#include <arch/tlbContext.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kflagManipulation.h>
	#include <__kclasses/bitmap.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/multipleReaderLock.h>
	#include <kernel/common/processId.h>
	#include <kernel/common/numaTypes.h>
	#include <kernel/common/taskTrib/prio.h>
	// Do not #include <kernel/common/process.h> within this file.

/**	EXPLANATION:
 * To spawn a new thread, first the caller must ask the thread's parent if there
 * are any remaining slots in the process's thread list. If there are none,
 * too bad.
 *
 * Assuming that the parent process was able to justify a new thread, it will
 * consume a new thread ID, and fill a new slot in its thread list, and assign
 * the new thread that thread ID. The parent process also fills out the new
 * thread's default values for affinity. The new thread is then returned to the
 * caller.
 *
 * Next, the caller must call initialize() on the new thread so that the
 * thread's register context, ring0 & 3 stacks, and possible TLB context can be
 * allocated. The caller must also fill out the new thread's scheduling
 * parameters.
 *
 * After that, the caller must call initialize() on the new thread's register
 * context and TLB context. This is to ensure that both structures are populated
 * with acceptable default values. At this point, the new thread is ready to be
 * executed.
 *
 * The caller now calls the Task Tributary's schedule() method, which takes the
 * new thread and adds it to one of its bound CPUs' queues. The new thread has
 * now been fully spawned and scheduled.
 *
 * This decision may change later, but for now, execution domain is a process
 * attribute, and not a thread specific attribute.
 **/

#define TASK_FLAGS_CUSTPRIO		(1<<0)

#define TASK_SCHEDFLAGS_SCHED_WAITING	(1<<0)

class processStreamC;
class cpuStreamC;

class taskC
{
public:
	enum schedStateE { DORMANT=1, RUNNABLE, RUNNING, UNSCHEDULED };
	enum schedPolicyE { ROUND_ROBIN=1, REAL_TIME };

	taskC(
		processId_t taskId, processStreamC *parentProcess,
		schedPolicyE schedPolicy, prio_t prio,
		uarch_t flags)
	:
		stack0(__KNULL), stack1(__KNULL),
		id(taskId), parent(parentProcess), flags(0),

		internalPrio(CC"Custom", prio),
		schedPolicy(schedPolicy), schedFlags(0),
		schedState(UNSCHEDULED),

		currentCpu(__KNULL),
		nLocksHeld(0)
	{
		if (__KFLAG_TEST(flags, TASK_FLAGS_CUSTPRIO))
		{
			__KFLAG_SET(this->flags, TASK_FLAGS_CUSTPRIO);
			schedPrio = &internalPrio;
		} else {
			schedPrio = &prioClasses[prio];
		};
	}

	error_t initialize(void);

	// Passes down parent attributes to child.
	error_t cloneStateIntoChild(taskC *child);

public:
	// Do *NOT* move 'stack' from where it is.
	void		*stack0, *stack1;

	// Basic information.
	processId_t		id;
	processStreamC		*parent;
	taskContextS		context;
	uarch_t			flags;
	multipleReaderLockC	lock;

	// Scheduling information.
	prioClassS	*schedPrio, internalPrio;
	schedPolicyE	schedPolicy;
	ubit8		schedOptions;
	ubit8		schedFlags;
	schedStateE	schedState;
	cpuStreamC	*currentCpu;

	// Miscellaneous properties (NUMA affinity, etc).
	ubit16		nLocksHeld;
#if __SCALING__ >= SCALING_SMP
	bitmapC		cpuAffinity;
#endif
#if __SCALING__ >= SCALING_CC_NUMA
	/* Denotes the default memory bank for this thread. When a thread is
	 * asks for memory, the kernel assigns it a default memory bank based
	 * on its CPU affinity. This is generally memory that is NUMA local.
	 **/
	sharedResourceGroupC<multipleReaderLockC, numaBankId_t>
		defaultMemoryBank;
#endif

#ifdef CONFIG_PER_TASK_TLB_CONTEXT
	tlbContextS	tlbContext;
#endif

	// Events being waited on by this thread.
	bitmapC		registeredEvents, pendingEvents;
};

#endif

