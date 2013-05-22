#ifndef _PROCESS_TRIBUTARY_H
	#define _PROCESS_TRIBUTARY_H

	#include <chipset/memory.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/wrapAroundCounter.h>
	#include <kernel/common/process.h>
	#include <kernel/common/task.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/multipleReaderLock.h>
	#include <kernel/common/processId.h>

#define PROCTRIB			"Proc Trib: "

#define PROCESSTRIB_UPDATE_ADD		0x0
#define PROCESSTRIB_UPDATE_SUBTRACT	0x1
#define PROCESSTRIB_UPDATE_SET		0x2

/**	Flag values for processTribC::spawnProcess().
 **/
#define SPAWNPROC_FLAGS_AFFINITY_STINHERIT	(0)
#define SPAWNPROC_FLAGS_AFFINITY_PINHERIT	(1<<0)

#define SPAWNPROC_FLAGS_SCHEDPRIO_PRIOCLASS	(0)
#define SPAWNPROC_FLAGS_SCHEDPRIO_SET		(1<<5)

class processTribC
:
public tributaryC
{
public:
	processTribC(void *vaddrSpaceBaseAddr, uarch_t vaddrSpaceSize)
	:
	__kprocess(
		0x0, 0x0, PROCESS_EXECDOMAIN_KERNEL,
		NUMABANKID_INVALID,
		vaddrSpaceBaseAddr, vaddrSpaceSize),
	// Kernel occupies process ID 0; we begin handing process IDs from 1.
	nextProcId(CHIPSET_MEMORY_MAX_NPROCESSES - 1, 1)
	{
		memset(processes.rsrc, 0, sizeof(processes.rsrc));
		processes.rsrc[0] = &__kprocess;

		fillOutPrioClasses();
	}

	error_t initialize(void)
	{
		/* DO NOT call __kprocess.initialize() here. It is explicitly
		 * called by __korientationInit().
		 **/
		return ERROR_SUCCESS;
	}

public:
	containerProcessC *__kgetStream(void) { return &__kprocess; };
	processStreamC *getStream(processId_t id);

	/**	EXPLANATION:
	 * Distributaries are by nature high privilege processes with high
	 * trust placed in them, and they all run in the kernel executaion
	 * domain. Their scheduler priority and policy are determined by
	 * the kernel and not by the spawning process, and they handle their own
	 * NUMA affinity on a per-thread basis.
	 *
	 * As you can see, the majority of the general userspace arguments
	 * to spawnStream() are redundant for creating distributary processes.
	 **/
	error_t spawnDistributary(
		utf8Char *commandLine,
		utf8Char *environment,
		numaBankId_t addrSpaceBinding,
		ubit8 schedPrio,
		uarch_t flags,
		distributaryProcessC **ret);

	error_t spawnStream(
		utf8Char *commandLine,
		utf8Char *environment,
		numaBankId_t addrSpaceBinding,	// NUMA addrspace binding.
		bitmapC *cpuAffinity,		// Ocean/NUMA/SMP affinity.
		void *elevation,		// Privileges.
		ubit8 execDomain,		// Kernel mode vs. User mode.
		ubit8 schedPolicy,		// Sched policy of 1st thread.
		ubit8 prio,			// Sched prio of 1st thread.
		uarch_t flags,			// proc + 1st thread spawn flags
		processStreamC **ret);		// Returned error value.

	error_t destroyStream(void);

private:
	void fillOutPrioClasses(void);
	error_t getNewProcessId(processId_t *ret);

private:
	containerProcessC	__kprocess;
	wrapAroundCounterC	nextProcId;
	struct
	{
		multipleReaderLockC	lock;
		processStreamC		*rsrc[CHIPSET_MEMORY_MAX_NPROCESSES];
	} processes;
};

extern processTribC	processTrib;

/**	Inline Methods
 *****************************************************************************/

inline processStreamC *processTribC::getStream(processId_t id)
{
	processStreamC	*ret;
	uarch_t		rwFlags;

	processes.lock.readAcquire(&rwFlags);
	ret = processes.rsrc[PROCID_PROCESS(id)];
	processes.lock.readRelease(rwFlags);

	return ret;
}

inline error_t processTribC::getNewProcessId(processId_t *ret)
{
	uarch_t		rwFlags;
	sbit32		tmpId;

	processes.lock.readAcquire(&rwFlags);
	tmpId = nextProcId.getNextValue((void **)processes.rsrc);
	processes.lock.readRelease(rwFlags);

	if (tmpId < 1) { return ERROR_RESOURCE_EXHAUSTED; };
	*ret = (processId_t)tmpId;
	return ERROR_SUCCESS;
}

#endif

