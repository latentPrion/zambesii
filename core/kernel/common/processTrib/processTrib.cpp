
#include <chipset/memory.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <__kthreads/__korientation.h>


processTribC::processTribC(void)
{
	processes.rsrc = __KNULL;
}

error_t processTribC::initialize(void)
{
	// Init kernel process.
	memset(&__kprocess, 0, sizeof(processS));

	__kprocess.id = 0x0;
	__kprocess.head = &__korientationThread;
	__kprocess.fileName = (utf16Char *)"";
	__kprocess.filePath = (utf16Char *)"";
	__kprocess.argString = (utf16Char *)"";
	__kprocess.env = (utf16Char *)"";
	__kprocess.memoryStream = &memoryTrib.__kmemoryStream;

	// Init __korientation thread.
	memset(&__korientationThread, 0, sizeof(__korientationThread));

	__korientationThread.id = 0x1;
	__korientationThread.parent = &__kprocess;
	__korientationThread.stacks.priv0 = __korientationStack;
	__korientationThread.nLocksHeld = 0;
	__korientationThread.smpConfig.last = CPUID_INVALID;
	// Init cpuConfig and numaConfig BMPs later.
	__korientationThread.numaConfig.def.rsrc =
		CHIPSET_MEMORY_NUMA___KSPACE_BANKID;

	return ERROR_SUCCESS;
}

processS *processTribC::spawn(const utf16Char *)
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


