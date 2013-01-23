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
	#include <kernel/common/memoryTrib/memoryStream.h>
	#include <kernel/common/timerTrib/timerStream.h>
	#include <__kthreads/__korientation.h>


#define PROCESS_EXECDOMAIN_KERNEL	0x1
#define PROCESS_EXECDOMAIN_USER		0x2

/**	Flags for processC::flags.
 **/
#define PROCESS_FLAGS___KPROCESS	(1<<0)

/**	Flags for processStreamC::spawnThread().
 **/
#define SPAWNTHREAD_FLAGS_FIRST_THREAD		(1<<0)
#define SPAWNTHREAD_FLAGS_AFFINITY_PINHERIT	(1<<1)
#define SPAWNTHREAD_FLAGS_SCHEDPOLICY_SET	(1<<2)
#define SPAWNTHREAD_FLAGS_SCHEDPOLICY_DEFAULT	(1<<3)
#define SPAWNTHREAD_FLAGS_SCHEDPOLICY_STINHERIT	(1<<4)
#define SPAWNTHREAD_FLAGS_SCHEDPRIO_SET		(1<<5)
#define SPAWNTHREAD_FLAGS_SCHEDPRIO_DEFAULT	(1<<6)
#define SPAWNTHREAD_FLAGS_SCHEDPRIO_STINHERIT	(1<<7)

#define SPAWNTHREAD_STATUS_NO_PIDS		0x1

class processStreamC
{
public:
	processStreamC(
		processId_t processId, processId_t parentProcId,
		ubit8 execDomain,
		pagingLevel0S *level0Accessor, paddr_t level0Paddr)
	:
	id(processId), flags(0), parentId(parentProcId),
	// Kernel process begins handing out thread IDs at 1 since 0 is taken.
	nextTaskId(
		CHIPSET_MEMORY_MAX_NTASKS - 1,
		(processId == __KPROCESSID) ? 1 : 0),
	commandLine(__KNULL), workingDir(__KNULL), fullName(__KNULL),
	argString(__KNULL), env(__KNULL),
	execDomain(execDomain),
	memoryStream(processId, level0Accessor, level0Paddr)
	{
		memset(tasks, 0, sizeof(tasks));
		defaultMemoryBank.rsrc = NUMABANKID_INVALID;

		if (id == __KPROCESSID)
		{
			__KFLAG_SET(flags, PROCESS_FLAGS___KPROCESS);
			tasks[0] = &__korientationThread;
			defaultMemoryBank.rsrc =
				CHIPSET_MEMORY_NUMA___KSPACE_BANKID;
		};
	}

	error_t initialize(
		const utf8Char *commandLine,
		const utf8Char *fullName,
		const utf8Char *workingDir);

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

	error_t spawnThread(
		void *entryPoint, taskC::schedPolicyE schedPolicy,
		ubit8 prio, uarch_t flags);
	
public:
	uarch_t			id;
	uarch_t			flags;
	processId_t		parentId;
	taskC			*tasks[CHIPSET_MEMORY_MAX_NTASKS];
	multipleReaderLockC	taskLock;
	wrapAroundCounterC	nextTaskId;

	utf8Char		*commandLine, *workingDir, *fullName;
	utf8Char		**argString, **env;

#if __SCALING__ >= SCALING_SMP
	bitmapC			cpuAffinity;
#endif
#if __SCALING__ >= SCALING_CC_NUMA
	sharedResourceGroupC<multipleReaderLockC, numaBankId_t>
		defaultMemoryBank;
#endif

	// Execution domain and privilege elevation.
	ubit8			execDomain;

#if __SCALING__ >= SCALING_SMP
	// Tells which CPUs this process has run on.
	bitmapC			cpuTrace;
#endif

	memoryStreamC		memoryStream;
	timerStreamC		timerStream;

private:
	sarch_t getNextThreadId(void);
	taskC *allocateNewThread(processId_t id);
	void removeThread(processId_t id);
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
	sarch_t			ret;

	ret = nextTaskId.getNextValue(reinterpret_cast<void **>( tasks ));
	if (ret == -1)
	{
		ret = nextTaskId.getNextValue(
			reinterpret_cast<void **>( tasks ));

		if (ret == -1) { return -1; };
	};
	return ret;
}

#endif

