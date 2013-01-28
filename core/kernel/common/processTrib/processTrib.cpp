
#include <chipset/memory.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/timerTrib/timerTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <__kthreads/__korientation.h>


prioClassS	prioClasses[PRIOCLASS_NCLASSES];

void processTribC::fillOutPrioClasses(void)
{
	new (&prioClasses[0]) prioClassS(CC"Idle", 0);
	new (&prioClasses[1]) prioClassS(CC"Low", 4);
	new (&prioClasses[2]) prioClassS(CC"Moderate", 8);
	new (&prioClasses[3]) prioClassS(CC"High", 12);
	new (&prioClasses[4]) prioClassS(CC"Critical", 16);
}

static inline error_t resizeAndMergeBitmaps(bitmapC *dest, bitmapC *src)
{
	error_t		ret;

	ret = dest->resizeTo(src->getNBits());
	if (ret != ERROR_SUCCESS) { return ret; };

	dest->merge(src);
	return ERROR_SUCCESS;
}

processStreamC *processTribC::spawnStream(
	const utf8Char *_commandLine,		// Full command line w/ args.
	bitmapC *cpuAffinity,			// Ocean/NUMA/SMP affinity.
	void */*elevation*/,			// Privileges.
	ubit8 execDomain,			// Kernel mode vs. User mode.
	uarch_t flags,				// Process spawn flags.
	ubit8 /*schedPolicy*/,			// Sched policy of 1st thread.
	ubit8 /*prio*/,				// Sched prio of 1st thread.
	uarch_t /*ftFlags*/,			// 1st thread spawn flags.
	error_t *err				// Returned error value.
	)
{
	processId_t		newId, parentId;
	sbit32			newIdTmp;
	uarch_t			rwFlags;
	processStreamC		*newProc=__KNULL;
	utf8Char		*fileName, *workingDir;
	/**	NOTES:
	 * This routine will essentially be the guiding hand to starting up
	 * process: finding the file in the VFS, then finding out what
	 * executable format it is encoded in, then calling the exec format
	 * parser to get an exec map, and do dynamic linking, then spawning
	 * all of its streams (Memory Stream, etc), then calling the Task Trib
	 * to spawn its first thread.
	 **/
	// Get a file descriptor for the executable.

	/* ...Eventually, you'll end up with a vfsFileC descriptor. Next stage
	 * will be to try to get a parser for the file, so you can know whether
	 * or not it's executable.
	 **/

	/* If you manage to get a parser, the next step would be to get a list
	 * all its dynamic dependencies.
	 *
	 * Assuming you get all of those, now you can allocate and initialize
	 * a new processC object for this new process and initialize its
	 * address space.
	 *
	 * Next, copy the executable image into the address space and relocate
	 * it and all libraries. Spawn a thread 0 with the entry point of the
	 * process as its entry point. That's all.
	 **/

	// Get process ID from counter.
	processes.lock.readAcquire(&rwFlags);
	newIdTmp = nextProcId.getNextValue(
		reinterpret_cast<void **>( processes.rsrc ));

	processes.lock.readRelease(rwFlags);

	if (newIdTmp < 0)
	{
		__kprintf(WARNING PROCTRIB"Out of process IDs.\n");
		*err = ERROR_GENERAL;
		return __KNULL;
	};

	newId = newIdTmp << PROCID_PROCESS_SHIFT;
	// Get parent (spawning) process' process ID.
	parentId = cpuTrib.getCurrentCpuStream()
		->taskStream.currentTask->parent->id;

#if 0
	// Call initialize().
	*err = newProc->initialize(_commandLine, fileName, workingDir);
	if (*err != ERROR_SUCCESS)
	{
		delete newProc;
		return __KNULL;
	};

	// Add the new process to the global process array.
	processes.lock.writeAcquire();
	processes.rsrc[PROCID_PROCESS(newId)] = newProc;
	processes.lock.writeRelease();

	// Assign exec domain.
	newProc->execDomain = execDomain;

	// Deal with affinity for the new process.
	if (__KFLAG_TEST(flags, PROCESSTRIB_SPAWN_FLAGS_PINHERIT_AFFINITY))
	{
		// Inherit affinity from spawning process.
		*err = resizeAndMergeBitmaps(
			&newProc->cpuAffinity,
			&cpuTrib.getCurrentCpuStream()
				->taskStream.currentTask->parent->cpuAffinity);
	}
	else if (__KFLAG_TEST(flags, PROCESSTRIB_SPAWN_FLAGS_STINHERIT_AFFINITY))
	{
		// Inherit affinity from spawning thread.
		*err = resizeAndMergeBitmaps(
			&newProc->cpuAffinity,
			&cpuTrib.getCurrentCpuStream()
				->taskStream.currentTask->cpuAffinity);
	}
	else
	{
		// If affinity argument is valid:
		if (cpuAffinity != __KNULL)
		{
			*err = resizeAndMergeBitmaps(
				&newProc->cpuAffinity, cpuAffinity);
		}
		else
		{
			/* Else pull "default" affinity of "all banks/cpus" from
			 * Memory trib and CPU trib.
			 **/
			// Size up and copy bitmaps: Online CPUs.
			*err = resizeAndMergeBitmaps(
				&newProc->cpuAffinity, &cpuTrib.onlineCpus);

			if (*err != ERROR_SUCCESS) { return __KNULL; };
		};
	};

	// TODO: Handle elevation.

	// 
#endif
	return newProc;
}

