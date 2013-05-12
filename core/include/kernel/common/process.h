#ifndef _PROCESS_H
	#define _PROCESS_H

	#include <scaling.h>
	#include <chipset/memory.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kflagManipulation.h>
	#include <__kclasses/bitmap.h>
	#include <__kclasses/wrapAroundCounter.h>
	#include <kernel/common/task.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/multipleReaderLock.h>
	#include <kernel/common/execDomain.h>
	#include <kernel/common/memoryTrib/memoryStream.h>
	#include <kernel/common/timerTrib/timerStream.h>
	#include <kernel/common/floodplainn/floodplainnStream.h>
	#include <__kthreads/__korientation.h>

/**	Flags for processC::flags.
 **/
#define PROCESS_FLAGS___KPROCESS	(1<<0)

/**	Flags for processStreamC::spawnThread().
 **/
// STINHERIT by default.
#define SPAWNTHREAD_FLAGS_AFFINITY_STINHERIT	(0)
// Implied if the cpuAffinity BMP != __KNULL && !PINHERIT
#define SPAWNTHREAD_FLAGS_AFFINITY_SET		(0)
#define SPAWNTHREAD_FLAGS_AFFINITY_PINHERIT	(1<<1)

// STINHERIT by default.
#define SPAWNTHREAD_FLAGS_SCHEDPOLICY_STINHERIT	(0)
#define SPAWNTHREAD_FLAGS_SCHEDPOLICY_SET	(1<<3)

// Set to PRIOCLASS_DEFAULT by default.
#define SPAWNTHREAD_FLAGS_SCHEDPRIO_CLASS_DEFAULT	(0)
#define SPAWNTHREAD_FLAGS_SCHEDPRIO_STINHERIT		(1<<7)
#define SPAWNTHREAD_FLAGS_SCHEDPRIO_PRIOCLASS_SET	(1<<5)
#define SPAWNTHREAD_FLAGS_SCHEDPRIO_SET			(1<<6)

// Set to prevent the thread from being scheduled automatically on creation.
#define SPAWNTHREAD_FLAGS_DORMANT			(1<<8)

#define SPAWNTHREAD_STATUS_NO_PIDS		0x1

class processStreamC
{
public:
	processStreamC(
		processId_t processId, processId_t parentProcId,
		ubit8 execDomain,
		pagingLevel0S *level0Accessor, paddr_t level0Paddr)
	:
		id(processId << PROCID_PROCESS_SHIFT),
		parentId(parentProcId << PROCID_PROCESS_SHIFT),
		flags(0),

		// Kernel process hands out thread IDs from 1 since 0 is taken.
		nextTaskId(
			CHIPSET_MEMORY_MAX_NTASKS - 1,
			(processId == __KPROCESSID) ? 1 : 0),
		nTasks(0),

		fullName(__KNULL), workingDirectory(__KNULL), fileName(__KNULL),
		nArgs(0), nEnvVars(0),
		argString(__KNULL), env(__KNULL),

		execDomain(execDomain),

		memoryStream(processId, level0Accessor, level0Paddr)
	{
		memset(tasks, 0, sizeof(tasks));
		defaultMemoryBank.rsrc = NUMABANKID_INVALID;

		if (id == __KPROCESSID)
		{
			__KFLAG_SET(flags, PROCESS_FLAGS___KPROCESS);
			nTasks = 1;
			tasks[0] = &__korientationThread;
			defaultMemoryBank.rsrc =
				CHIPSET_MEMORY_NUMA___KSPACE_BANKID;
		};
	}

	error_t initialize(const utf8Char *commandLine, bitmapC *cpuAffinity);

	~processStreamC(void);

	error_t cloneStateIntoChild(processStreamC *child);
	error_t initializeFirstThread(
		taskC *task, taskC *spawningThread,
		taskC::schedPolicyE policy, ubit8 prio, uarch_t flags);

	error_t initializeChildThread(
		taskC *task, taskC *spawningThread,
		taskC::schedPolicyE policy, ubit8 prio, uarch_t flags);

public:
	taskC *getTask(processId_t processId);
	taskC *getThread(processId_t processId) { return getTask(processId); }

	// I am very reluctant to have this "argument" parameter.
	error_t spawnThread(
		void (*entryPoint)(void *),
		void *argument,
		bitmapC *cpuAffinity,
		taskC::schedPolicyE schedPolicy, ubit8 prio,
		uarch_t flags,
		processId_t *ret);

public:
	processId_t		id, parentId;
	uarch_t			flags;

	multipleReaderLockC	taskLock;
	wrapAroundCounterC	nextTaskId;
	uarch_t			nTasks;
	taskC			*tasks[CHIPSET_MEMORY_MAX_NTASKS];

	utf8Char		*fullName, *workingDirectory, *fileName;
	ubit8			nArgs, nEnvVars;
	utf8Char		**argString, **env;

	// Execution domain and privilege elevation information.
	ubit8			execDomain;

#if __SCALING__ >= SCALING_SMP
	// Tells us which CPUs this process has run on.
	bitmapC			cpuTrace;
	bitmapC			cpuAffinity;
#endif
#if __SCALING__ >= SCALING_CC_NUMA
	sharedResourceGroupC<multipleReaderLockC, numaBankId_t>
		defaultMemoryBank;
#endif

	// Events which have been queued on this process.
	bitmapC			pendingEvents;

	memoryStreamC		memoryStream;
	timerStreamC		timerStream;
	floodplainnStreamC	floodplainnStream;

private:
	sarch_t getNextThreadId(void);
	taskC *allocateNewThread(processId_t newThreadId);
	void removeThread(processId_t id);

	void __kprocessAllocateInternals(void);
	void __kprocessGenerateArgString(void);
	void __kprocessGenerateEnvString(void);
	void __kprocessInitializeBmps(void);

	error_t allocateInternals(void);
	error_t generateArgString(void);
	error_t generateEnvString(void);
	error_t initializeBmps(void);
};

class containedProcessC
:
public processStreamC
{
};

class containerProcessC
:
public processStreamC
{
	containerProcessC(
		processId_t processId, processId_t parentProcessId,
		ubit8 execDomain,
		pagingLevel0S *level0Accessor, paddr_t level0Paddr)
	:
	processStreamC(
		processId, parentProcessId, execDomain,
		level0Accessor, level0Paddr)
	{}

	error_t initialize(void);
	~containerProcessC(void) {}
};

/**	Inline Methods:
 *****************************************************************************/

inline taskC *processStreamC::getTask(processId_t id)
{
	taskC		*ret;
	uarch_t		rwFlags;

	taskLock.readAcquire(&rwFlags);
	ret = tasks[PROCID_TASK(id)];
	taskLock.readRelease(rwFlags);

	return ret;
}

inline sarch_t processStreamC::getNextThreadId(void)
{
	return nextTaskId.getNextValue(reinterpret_cast<void **>( tasks ));
}

#endif

