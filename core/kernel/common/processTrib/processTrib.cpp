
#include <chipset/memory.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <__kthreads/__korientation.h>


// Global array of processes.
sharedResourceGroupC<multipleReaderLockC, processStreamC **>	processes;

processTribC::processTribC(void)
:
__kprocess(0x0, 0x0), nextProcId(CHIPSET_MEMORY_MAX_NPROCESSES - 1)
{
}

error_t processTribC::initialize(void)
{
	__kprocess.tasks[0] = &__korientationThread;
	__kprocess.absName = (utf8Char *)":ekfs/zambezii.zxe";
	__kprocess.argString = __KNULL;
	__kprocess.env = __KNULL;
	__kprocess.memoryStream = &memoryTrib.__kmemoryStream;

	

	// Init __korientation thread.
	memset(&__korientationThread, 0, sizeof(__korientationThread));

	__korientationThread.id = 0x0;
	__korientationThread.parent = &__kprocess;
	__korientationThread.stack0 = __korientationStack;
	__korientationThread.nLocksHeld = 0;
	// Init cpuConfig and numaConfig BMPs later.
	__korientationThread.localAffinity.def.rsrc =
		CHIPSET_MEMORY_NUMA___KSPACE_BANKID;

	__kprocess.initMagic = PROCESS_INIT_MAGIC;

	return ERROR_SUCCESS;
}

error_t processTribC::initialize2(void)
{
	// Allocate the array of processes.
	processes.rsrc = new processStreamC *[CHIPSET_MEMORY_MAX_NPROCESSES];
	if (processes.rsrc == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};

	memset(
		processes.rsrc, 0,
		sizeof(void *) * CHIPSET_MEMORY_MAX_NPROCESSES);

	processes.rsrc[0] = &__kprocess;
	return ERROR_SUCCESS;
}

processStreamC *processTribC::spawn(
	const utf8Char *_commandLine,		// Full command line w/ args.
	affinityS *affinity,			// Ocean/NUMA/SMP affinity.
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
	processStreamC		*newProc;
	utf8Char		*absName, *workingDir;
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

	newProc = new processStreamC(newId, parentId);
	if (newProc == __KNULL)
	{
		*err = ERROR_MEMORY_NOMEM;
		return __KNULL;
	};

	// Call initialize().
	*err = newProc->initialize(_commandLine, absName, workingDir);
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
		*err = affinity::copyMachine(&newProc->affinity, affinity);
	}
	else if (__KFLAG_TEST(flags, PROCESSTRIB_SPAWN_FLAGS_STINHERIT_AFFINITY))
	{
		// Inherit affinity from spawning thread.
		newProc->affinity.machines = new localAffinityS;
		if (newProc->affinity.machines == __KNULL)
		{
			*err = ERROR_MEMORY_NOMEM;
			return __KNULL;
		};

		newProc->affinity.nEntries = 1;
		*err = affinity::copyLocal(
			newProc->affinity.machines,
			&cpuTrib.getCurrentCpuStream()
				->taskStream.currentTask->localAffinity);
	}
	else
	{
		// If affinity argument is valid:
		if (affinity != __KNULL)
		{
			*err = affinity::copyMachine(
				&newProc->affinity, affinity);
		}
		else
		{
			/* Else pull "default" affinity of "all banks/cpus" from
			 * Memory trib and CPU trib.
			 **/
			newProc->affinity.machines = new localAffinityS;
			if (newProc->affinity.machines == __KNULL)
			{
				*err = ERROR_MEMORY_NOMEM;
				return __KNULL;
			};

			newProc->affinity.nEntries = 1;

			// Size up and copy bitmaps: Online CPUs.
			*err = newProc->affinity.machines->cpus.initialize(
				cpuTrib.onlineCpus.getNBits());

			if (*err != ERROR_SUCCESS) {
				return __KNULL;
			};

			newProc->affinity.machines->cpus.merge(
				&cpuTrib.onlineCpus);

			// Online CPU banks.
			*err = newProc->affinity.machines->cpuBanks.initialize(
				cpuTrib.availableBanks.getNBits());

			if (*err != ERROR_SUCCESS) {
				return __KNULL;
			};

			newProc->affinity.machines->cpuBanks.merge(
				&cpuTrib.availableBanks);

			// Online memory banks.
			*err = newProc->affinity.machines->memBanks.initialize(
				memoryTrib.availableBanks.getNBits());

			if (*err != ERROR_SUCCESS) {
				return __KNULL;
			};

			newProc->affinity.machines->memBanks.merge(
				&memoryTrib.availableBanks);
		};
	};

	// TODO: Handle elevation.

	// 

	return newProc;
}

