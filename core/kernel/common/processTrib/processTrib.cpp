
#include <chipset/memory.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kstdlib/__kclib/string.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <__kthreads/__korientation.h>


// Global array of processes.
sharedResourceGroupC<multipleReaderLockC, processS **>	processes;

processTribC::processTribC(void)
{
	nextProcId.initialize(CHIPSET_MAX_NPROCESSES - 1);
}

error_t processTribC::initialize(void)
{
	// Init kernel process.
	memset(&__kprocess, 0, sizeof(processS));

	__kprocess.id = 0x0;
	__kprocess.tasks[1] = &__korientationThread;
	__kprocess.fileName = (utf8Char *)"zambezii.zxe";
	__kprocess.filePath = (utf8Char *)":ekfs/zambezii/core";
	__kprocess.argString = (utf8Char *)"-debug=1";
	__kprocess.env = (utf8Char *)"";
	__kprocess.memoryStream = &memoryTrib.__kmemoryStream;

	// Init __korientation thread.
	memset(&__korientationThread, 0, sizeof(__korientationThread));

	__korientationThread.id = 0x1;
	__korientationThread.parent = &__kprocess;
	__korientationThread.stack = __korientationStack;
	__korientationThread.nLocksHeld = 0;
	__korientationThread.smpConfig.last = CPUID_INVALID;
	// Init cpuConfig and numaConfig BMPs later.
	__korientationThread.numaConfig.def.rsrc =
		CHIPSET_MEMORY_NUMA___KSPACE_BANKID;

	return ERROR_SUCCESS;
}

error_t processTribC::initialize2(void)
{
	// Allocate the array of processes.
	processes.rsrc = new processS *[CHIPSET_MAX_NPROCESSES];
	if (processes.rsrc == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};

	memset(processes.rsrc, 0, sizeof(void *) * CHIPSET_MAX_NPROCESSES);
	return ERROR_SUCCESS;
}

processS *processTribC::spawn(const utf8Char *)
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


