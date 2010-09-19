
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <__kthreads/__korientation.h>


processTribC::processTribC(void)
{
}

error_t processTribC::initialize(void)
{
	// Init kernel process.
	memset(&__kprocess, 0, sizeof(processS));

	__kprocess.head = &__korientationThread;
	__kprocess.fileName = const_cast<char *>( "zambezii.zxe" );
	__kprocess.filePath = const_cast<char *>( "/zambezii/core" );

	__kprocess.memoryStream = &memoryTrib.__kmemoryStream;

	// Init __korientation thread.
	memset(&__korientationThread, 0, sizeof(__korientationThread));

	__korientationThread.id = 0x1;
	__korientationThread.parent = &__kprocess;
	__korientationThread.stacks.priv0 = __korientationStack;
	__korientationThread.nLocksHeld = 0;

	/* The smpConfig, numaConfig and cpuTrace members will have to be left
	 * until later since they all either are, or contain bitmapC objects.
	 *
	 * Only thing that needs to be done is set numaConfig to default to
	 * the configured __kspace bank's ID.
	 *
	 * bitmapC depends on dynamic memory allocation from the numaTrib, no
	 * less. So we'll need to arrange to have a postInit() function which
	 * calls initialize on all of these items.
	 **/
	__korientationThread.numaConfig.def.rsrc =
		CHIPSET_MEMORY_NUMA___KSPACE_BANKID;

	return ERROR_SUCCESS;
}

processS *processTribC::spawn()
{
	/**	NOTES:
	 * This routine will essentially be the guiding hand to starting up
	 * process: finding the file in the VFS, then finding out what
	 * executable format it is encoded in, then calling the exec format
	 * parser to get an exec map, and do dynamic linking, then spawning
	 * all of its streams (Memory Stream, etc), then calling the Task Trib
	 * to spawn its first thread.
	 **/
}


