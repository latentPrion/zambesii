#ifndef _THREAD_H
	#define _THREAD_H

	#include <arch/registerContext.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kflagManipulation.h>
	#include <__kclasses/bitmap.h>
	#include <kernel/common/processId.h>
	#include <kernel/common/multipleReaderLock.h>
	#include <kernel/common/messageStream.h>
	#include <kernel/common/smpTypes.h>
	#include <kernel/common/taskTrib/prio.h>
	#include <__kthreads/__korientation.h>


#define TASK				"Task "

#define TASK_FLAGS_CUSTPRIO		(1<<0)

#define TASK_SCHEDFLAGS_SCHED_WAITING	(1<<0)

class CpuStream;
class ProcessStream;
class RegisterContext;
class Thread;

/**	Base class with scheduling info: Task.
 ******************************************************************************/

class _Task
: protected Stream<Thread>
{
friend class ProcessStream;
public:
	enum schedPolicyE { INVALID=0, ROUND_ROBIN, REAL_TIME };

	_Task(
		Thread *parent, processId_t tid, bspPlugTypeE bpt,
		void *privateData);

	error_t initialize(void) { return ERROR_SUCCESS; }

	virtual ~_Task(void) {}

	void *getPrivateData(void) { return privateData; }

private:

	// General.
	bspPlugTypeE		bspPlugType;
	void			*privateData;

public:
	// Scheduler related.
	sPriorityClass		*schedPrio, internalPrio;
	schedPolicyE		schedPolicy;
	ubit8			schedOptions;
	ubit8			schedFlags;

private:
	void inheritSchedPolicy(schedPolicyE schedPolicy, uarch_t flags);
	void inheritSchedPrio(prio_t prio, uarch_t flags);

protected:
friend class _TaskContext;
	ubit8 isBspFirstPlugTask(void)
		{ return bspPlugType == BSP_PLUGTYPE_FIRSTPLUG; }
	ubit8 isBspPowerTask(void) { return bspPlugType > BSP_PLUGTYPE_NOTBSP; }
	ubit8 isPowerTask(void)
		{ return (PROCID_PROCESS(id) == CPU_PROCESSID); }
};

/**	EXPLANATION:
 * TaskContext holds the information that would enable a thread to be
 * split such that its code can be executed on multiple CPUs, as
 * per-cpu-threads.
 *
 * In other words, all the member objects and functions in this class
 * are those which are unique to a running thread instance. We split
 * these members off into a separate set of context variables to enable
 * us to have multiple CPUs executing a single thread without any risk
 * of all those CPUs trampling that single thread's contextual data.
 ******************************************************************************/
class Thread;

class _TaskContext
:	protected Stream<Thread>
{
friend class ProcessStream;
public:
	enum runStateE { UNSCHEDULED=1, RUNNABLE, RUNNING, STOPPED };
	enum blockStateE {
		BLOCKED_UNSCHEDULED=1, PREEMPTED, DORMANT, BLOCKED };

protected:
	_TaskContext(Thread *parent, processId_t tid, bspPlugTypeE bspPlugType)
	: Stream<Thread>(parent, tid),
	bspPlugType(bspPlugType),
	runState(UNSCHEDULED), blockState(BLOCKED_UNSCHEDULED),
	nLocksHeld(0), context(NULL)
	{
#if __SCALING__ >= SCALING_CC_NUMA
		defaultMemoryBank.rsrc = NUMABANKID_INVALID;
#endif

		memset(
			bspPowerTaskContextCpuAffinityMem, 0,
			sizeof(bspPowerTaskContextCpuAffinityMem));

		if (isPowerTaskContext()) {
			inheritAffinity(NULL, 0);
		};
	}

	error_t initialize(void);

public:
	// Scheduler related.
	bspPlugTypeE		bspPlugType;
	runStateE		runState;
	blockStateE		blockState;

	// Miscellaneous.
	ubit16			nLocksHeld;
	RegisterContext		*context;
#ifdef CONFIG_PER_TASK_TLB_CONTEXT
	sTlbContext		tlbContext;
#endif
#if __SCALING__ >= SCALING_SMP
	Bitmap			cpuAffinity;
#endif
#if __SCALING__ >= SCALING_CC_NUMA
	/* Denotes the default memory bank for this thread. When a thread is
	 * asks for memory, the kernel assigns it a default memory bank based
	 * on its CPU affinity. This is generally memory that is NUMA local.
	 **/
	SharedResourceGroup<MultipleReaderLock, numaBankId_t>
		defaultMemoryBank;
#endif
protected:
	static ubit8			bspPowerTaskContextCpuAffinityMem[32];

	ubit8 isBspFirstPlugTaskContext(void)
		{ return bspPlugType == BSP_PLUGTYPE_FIRSTPLUG; }
	ubit8 isBspPowerTaskContext(void)
		{ return bspPlugType > BSP_PLUGTYPE_NOTBSP; }
	ubit8 isPowerTaskContext(void)
		{ return (PROCID_PROCESS(id) == CPU_PROCESSID); }

private:
	void initializeRegisterContext(
		void (*entryPoint)(void *), void *stack0, void *stack1,
		ubit8 execDomain,
		sarch_t isFirstThread);

#if __SCALING__ >= SCALING_SMP
	error_t inheritAffinity(Bitmap *cpuAffinity, uarch_t flags);
#endif
};

/**	Class Thread, a normal thread.
 ******************************************************************************/

extern CpuStream	bspCpu;

class Thread
:	public Stream<ProcessStream>, public _Task, public _TaskContext
{
friend class ProcessStream;
public:
	Thread(
		processId_t id, ProcessStream *parent, bspPlugTypeE bspPlugType,
		void *privateData);

	error_t initialize(void)
	{
		error_t		ret;

		ret = _Task::initialize();
		if (ret != ERROR_SUCCESS) { return ret; };
		ret = _TaskContext::initialize();
		if (ret != ERROR_SUCCESS) { return ret; };
		// Initialize callback queues.
		return messageStream.initialize();
	}

public:
	processId_t getFullId(void) { return id; }

	ubit8 isBspFirstPlugThread(void)
		{ return bspPlugType == BSP_PLUGTYPE_FIRSTPLUG; }
	ubit8 isBspPowerThread(void) { return _Task::isBspPowerTask(); }
	ubit8 isPowerThread(void) { return _Task::isPowerTask(); }

private:
	// Allocates stacks for kernelspace and userspace (if necessary).
	error_t allocateStacks(void);

private:processId_t		id;
public:
	bspPlugTypeE		bspPlugType;
	ProcessStream		*parent;
	CpuStream		*currentCpu;
	void			*stack0, *stack1;

	// Asynchronous API message queues for this thread.
	MessageStream		messageStream;
};

#endif

