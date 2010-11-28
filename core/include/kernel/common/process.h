#ifndef _PROCESS_H
	#define _PROCESS_H

	#include <chipset/memory.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/bitmap.h>
	#include <__kclasses/wrapAroundCounter.h>
	#include <kernel/common/task.h>
	#include <kernel/common/machineAffinity.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/multipleReaderLock.h>
	#include <kernel/common/memoryTrib/memoryStream.h>

#define PROCESS_INIT_MAGIC	0x1D0C3551

#define PROCESS_EXECDOMAIN_KERNEL	0x1
#define PROCESS_EXECDOMAIN_USER		0x2

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
	processStreamC(processId_t id, processId_t parent);
	error_t initialize(utf8Char *absName);
	~processStreamC(void);

	error_t initializeChild(processStreamC *child);
	error_t initializeFirstThread(
		taskC *task, taskC *spawningThread,
		ubit8 policy, ubit8 prio, uarch_t flags);

	error_t initializeChildThread(
		taskC *task, taskC *spawningThread,
		ubit8 policy, ubit8 prio, uarch_t flags);

public:
	taskC *getTask(processId_t processId);

	error_t spawnThread(
		void *entryPoint, ubit8 schedPolicy, ubit8 prio, uarch_t flags);
	
public:
	uarch_t			id;
	processId_t		parentId;
	taskC			*tasks[CHIPSET_MEMORY_MAX_NTASKS];
	multipleReaderLockC	taskLock;

	utf8Char		*absName, *argString, *env;

	// Oceann and local affinity.
	affinityS		affinity;
	// Don't forget to initialize new processes' localAffinity pointer.
	localAffinityS		*localAffinity;

	// Execution domain and privilege elevation.
	ubit8			execDomain;

	// Tells which CPUs this process has run on.
	bitmapC			cpuTrace;
	wrapAroundCounterC	nextTaskId;
	ubit32			initMagic;

	memoryStreamC		*memoryStream;

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

