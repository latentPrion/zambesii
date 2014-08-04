#ifndef _THREAD_H
	#define _THREAD_H

	#include <arch/registerContext.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/bitmap.h>
	#include <kernel/common/processId.h>
	#include <kernel/common/taskTrib/prio.h>
	#include <kernel/common/multipleReaderLock.h>
	#include <kernel/common/messageStream.h>
	#include <__kthreads/__korientation.h>


#define TASK				"Task "

#define TASK_FLAGS_CUSTPRIO		(1<<0)

#define TASK_SCHEDFLAGS_SCHED_WAITING	(1<<0)

class CpuStream;
class ProcessStream;
class RegisterContext;
class TaskContext;

// Task and TaskContext instances can be unique or per-cpu.
namespace task { enum typeE { UNIQUE=1, PER_CPU }; }

/**	Base class with scheduling info: Task.
 ******************************************************************************/

class Task
{
friend class ProcessStream;
public:
	enum schedPolicyE { INVALID=0, ROUND_ROBIN, REAL_TIME };

	Task(ProcessStream *parent, void *privateData)
	:
	parent(parent), flags(0), privateData(privateData),

	// Usually overridden immediately by inheritSchedPrio(), though.
	schedPrio(&internalPrio),
	internalPrio(CC"Custom", PRIOCLASS_DEFAULT),
	schedPolicy(ROUND_ROBIN), schedOptions(0), schedFlags(0)
	{}

	error_t initialize(void) { return ERROR_SUCCESS; }

	virtual ~Task(void) {}

public:
	// virtual processId_t getFullId(void)=0;
	virtual task::typeE getType(void)=0;
	void *getPrivateData(void) { return privateData; }

private:
	void inheritSchedPolicy(schedPolicyE schedPolicy, uarch_t flags);
	void inheritSchedPrio(prio_t prio, uarch_t flags);

public:
	// General.
	ProcessStream		*parent;
	uarch_t			flags;
	void			*privateData;

	// Scheduler related.
	sPriorityClass		*schedPrio, internalPrio;
	schedPolicyE		schedPolicy;
	ubit8			schedOptions;
	ubit8			schedFlags;
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

class TaskContext
{
friend class ProcessStream;
public:
	enum runStateE { UNSCHEDULED=1, RUNNABLE, RUNNING, STOPPED };
	enum blockStateE {
		BLOCKED_UNSCHEDULED=1, PREEMPTED, DORMANT, BLOCKED };

	TaskContext(task::typeE contextType, void *parent)
	:
	contextType(contextType),
	runState(UNSCHEDULED), blockState(BLOCKED_UNSCHEDULED),
	nLocksHeld(0), context(NULL),

	/**	CAVEAT:
	 * When this class is instantiated as part of a per-cpu-thread
	 * setup (i.e, if it is a member object of a CPU Stream), it
	 * must always be set to the address of the current per-cpu-
	 * thread being executed by that CPU.
	 *
	 * (A CPU will only ever have one per-cpu-thread scheduled to
	 * it at a time.)
	 **/
	messageStream(this)
	{
		if (contextType == task::UNIQUE) {
			this->parent.thread = (Thread *)parent;
		} else {
			this->parent.cpu = (CpuStream *)parent;
		};

#if __SCALING__ >= SCALING_CC_NUMA
		defaultMemoryBank.rsrc = NUMABANKID_INVALID;
#endif
	}

	error_t initialize(void);

private:
	void initializeRegisterContext(
		void (*entryPoint)(void *), void *stack0, void *stack1,
		ubit8 execDomain,
		sarch_t isFirstThread);

#if __SCALING__ >= SCALING_SMP
	error_t inheritAffinity(Bitmap *cpuAffinity, uarch_t flags);
#endif

public:
	// General.
	/* If this is per-cpu context, then 'parentCpu' is valid and
	 * 'parentTask' is invalid. If this is thread context, then
	 * 'parentCpu' is invalid and 'parentTask' is valid.
	 **/
	/* Whether this object is embedded in a schedulable unique
	 * thread object, or in a CPU Stream object (as per-cpu thread
	 * context).
	 **/
	task::typeE		contextType;
	union
	{
		CpuStream	*cpu;
		Thread		*thread;
	} parent;

	// Scheduler related.
	runStateE		runState;
	blockStateE		blockState;

	// Miscellaneous.
	ubit16			nLocksHeld;
	RegisterContext	*context;
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

	// Asynchronous API message queues for this thread.
	MessageStream		messageStream;
};

/**	Class Thread, a normal thread.
 ******************************************************************************/

class Thread
:
public Task
{
friend class ProcessStream;
public:
	Thread(processId_t id, ProcessStream *parent, void *privateData)
	:
	Task(parent, privateData),
	id(id),
	Currenttpu(NULL),
	stack0(NULL), stack1(NULL),
	// For a normal thread, "Currenttpu" and "stack0" start as NULL.
	taskContext(task::UNIQUE, this)
	{
		if (this != &__korientationThread) { return; }

		// Set some hardcoded state in the orientation thread.
		stack0 = __korientationStack;
		//Currenttpu = &bspCpu;
	}

	error_t initialize(void)
	{
		error_t		ret;

		ret = Task::initialize();
		if (ret != ERROR_SUCCESS) { return ret; };

		return taskContext.initialize();
	}

public:
	processId_t getFullId(void) { return id; }
	TaskContext *getTaskContext(void) { return &taskContext; }
	virtual task::typeE getType(void) { return task::UNIQUE; }

private:
	// Allocates stacks for kernelspace and userspace (if necessary).
	error_t allocateStacks(void);

private:processId_t		id;
public:	CpuStream		*Currenttpu;
	void			*stack0, *stack1;

private:
	TaskContext		taskContext;
};

/**	Class PerCpuThread, an abstraction of per-cpu threads.
 ******************************************************************************/

class PerCpuThread
:
public Task
{
public:
	PerCpuThread(ProcessStream *parent, void *privateData)
	:
	Task(parent, privateData)
	{}

	error_t initialize(void) { return Task::initialize(); }

public:
	virtual task::typeE getType(void) { return task::PER_CPU; }
};

#endif

