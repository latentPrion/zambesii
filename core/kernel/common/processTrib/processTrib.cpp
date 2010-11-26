
#include <chipset/memory.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kstdlib/__kclib/string.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <__kthreads/__korientation.h>


// Global array of processes.
sharedResourceGroupC<multipleReaderLockC, processStreamC **>	processes;

processTribC::processTribC(void)
:
__kprocess(0)
{
	nextProcId.initialize(CHIPSET_MAX_NPROCESSES - 1);
}

error_t processTribC::initialize(void)
{
	__kprocess.tasks[1] = &__korientationThread;
	__kprocess.absName = (utf8Char *)":ekfs/core/zambezii.zxe";
	__kprocess.argString = (utf8Char *)"-debug=1";
	__kprocess.env = (utf8Char *)"";
	__kprocess.memoryStream = &memoryTrib.__kmemoryStream;

	// Init __korientation thread.
	memset(&__korientationThread, 0, sizeof(__korientationThread));

	__korientationThread.id = 0x1;
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
	processes.rsrc = new processStreamC *[CHIPSET_MAX_NPROCESSES];
	if (processes.rsrc == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};

	memset(processes.rsrc, 0, sizeof(void *) * CHIPSET_MAX_NPROCESSES);
	return ERROR_SUCCESS;
}

processStreamC *processTribC::spawn(const utf8Char *)
{
	/**	NOTES:
	 * This routine will essentially be the guiding hand to starting up
	 * process: finding the file in the VFS, then finding out what
	 * executable format it is encoded in, then calling the exec format
	 * parser to get an exec map, and do dynamic linking, then spawning
	 * all of its streams (Memory Stream, etc), then calling the Task Trib
	 * to spawn its first thread.
	 **/
	return __KNULL;
}

