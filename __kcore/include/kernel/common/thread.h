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
	#include <kernel/common/floodplainn/region.h>
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

	_Task(processId_t tid, Thread *parent, void *privateData);
	error_t initialize(void) { return ERROR_SUCCESS; }
	virtual ~_Task(void) {}

	void *getPrivateData(void) { return privateData; }

	void setSchedOptions(uarch_t opts)
		{ schedOptions = opts; }

	void setSchedOptionBits(uarch_t mask)
		{ FLAG_SET(schedOptions, mask); }

	void unsetSchedOptionBits(uarch_t mask)
		{ FLAG_UNSET(schedOptions, mask); }

private:

	// General.
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
	enum schedStateE
	{
		INVALID=0,
		UNSCHEDULED,
		DORMANT,
		RUNNABLE,
		RUNNING,
		BLOCKED,
		SHUTTING_DOWN,
		ZOMBIE
	};

	static utf8Char *schedStates[8];

protected:
	_TaskContext(processId_t tid, Thread *parent);
	error_t initialize(void);

public:
	/**	EXPLANATION:
	 * This is used to protect against two problems:
	 * 1. Multiple readers/writers accessing the scheduling state.
	 * 2. It's used to prevent lost wakeups.
	 *
	 * When a thread is assigned to wait on some object, that object must
	 * take a handle to said thread's schedState lock. Whenever someone
	 * wishes to signal the object, they must readAcquire() said waiting
	 * thread's schedState lock.
	 *
	 * Whenever the owning thread is about to check the state of one or more
	 * objects, it must writeAcquire() this lock and if the object has no
	 * data/messages available for processing, then the owning thread must
	 * pass this lock to the block() invocation __still acquired__. The
	 * block() invocation will writeRelease() the lock internally on behalf
	 * of the owning thread.
	 */
	struct SchedState
	{
		SchedState(void)
		: status(UNSCHEDULED), currentCpu(NULL)
		{}

		schedStateE	status;
		CpuStream	*currentCpu;
	};
	SharedResourceGroup<MultipleReaderLock, SchedState>	schedState;

	// Miscellaneous.
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

class Thread
:	public Stream<ProcessStream>, public _Task, public _TaskContext
{
friend class ProcessStream;
public:
	Thread(
		processId_t id, ProcessStream *parent, void *privateData,
		CpuStream *parentCpu=NULL);

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

	sbit8 shouldPreemptCurrentThreadOn(CpuStream *cpu);

	static sbit8 isBspPowerThread(processId_t tid);
	static sbit8 isPowerThread(processId_t tid)
	{
		// The thread with CPUID_INVALID is the BSP power thread.
		return tid == CPUID_INVALID
			|| PROCID_PROCESS(tid) == PROCID_PROCESS(CPU_PROCESSID);
	}

	void setRegion(fplainn::Region *region)
		{ this->region = region; };

	fplainn::Region *getRegion(void)
		{ return region; }

	void setResponse(MessageStream::sHeader *responseMessage)
		{ this->responseMessage = responseMessage; }

	error_t sendAckToSpawner(error_t err);

private:
	// Allocates stacks for kernelspace and userspace (if necessary).
	error_t allocateStacks(void);

private:processId_t				id;
public:
	ProcessStream				*parent;
	// Only valid for region threads of drivers.
	fplainn::Region				*region;
	CpuStream				*parentCpu;
	void					*stack0, *stack1;

	// Asynchronous API message queues for this thread.
	MessageStream				messageStream;
	MessageStream::sHeader		*responseMessage;
};

#endif

