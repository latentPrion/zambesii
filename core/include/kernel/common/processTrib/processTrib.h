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
// Inherit affinity from Spawning Thread.
#define PROCESSTRIB_SPAWN_FLAGS_STINHERIT_AFFINITY	(1<<0)
// Inherit affinity from spawning thread's process object.
#define PROCESSTRIB_SPAWN_FLAGS_PINHERIT_AFFINITY	(1<<1)
// Inherit elevation from spawning thread's process object.
#define PROCESSTRIB_SPAWN_FLAGS_INHERIT_ELEVATION	(1<<2)
// Inherit exec domain from spawning thread's process object.
#define PROCESSTRIB_SPAWN_FLAGS_INHERIT_EXECDOMAIN	(1<<3)

class processTribC
:
public tributaryC
{
public:
	processTribC(pagingLevel0S *level0Accessor, paddr_t level0Paddr)
	:
	__kprocess(0x0, 0x0, PROCESS_EXECDOMAIN_KERNEL,
		level0Accessor, level0Paddr),
	// Kernel occupies process ID 0; we begin handing process IDs from 1.
	nextProcId(CHIPSET_MEMORY_MAX_NPROCESSES - 1, 1)
	{
		memset(processes.rsrc, 0, sizeof(processes.rsrc));
		processes.rsrc[0] = &__kprocess;
	}

	error_t initialize(void) { return ERROR_SUCCESS; }

public:
	processStreamC *__kgetStream(void) { return &__kprocess; };
	processStreamC *getStream(processId_t id);

	processStreamC *spawnStream(
		const utf8Char *_commandLine,	// Full command line w/ args.
		bitmapC *cpuAffinity,		// Ocean/NUMA/SMP affinity.
		void *elevation,		// Privileges.
		ubit8 execDomain,		// Kernel mode vs. User mode.
		uarch_t flags,			// Process spawn flags.
		ubit8 schedPolicy,		// Sched policy of 1st thread.
		ubit8 prio,			// Sched prio of 1st thread.
		uarch_t ftFlags,		// 1st thread spawn flags.
		error_t *err);			// Returned error value.

	error_t destroyStream(void);

public:
	processStreamC		__kprocess;

private:
	wrapAroundCounterC	nextProcId;
	struct
	{
		multipleReaderLockC	lock;
		processStreamC		*rsrc[CHIPSET_MEMORY_MAX_NPROCESSES];
	} processes;
};

extern processTribC	processTrib;
#if __SCALING__ >= SCALING_SMP
extern ubit8		__kprocessCpuAffinityBmpMem[];
extern ubit8		__kprocessCpuTraceBmpMem[];
#endif


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

#endif

