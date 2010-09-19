
#include <__kstdlib/__kclib/string.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <__kthreads/__korientation.h>


cpuTribC::cpuTribC(void)
{
}

error_t cpuTribC::initialize(void)
{
	memset(&__korientationThread, 0, sizeof(__korientationThread));

	__korientationThread.id = 0x1;
	__korientationThread.parent = processTrib.__kgetProcess();
	__korientationThread.stacks.priv0 = __korientationStack;
	__korientationThread.nLocksHeld = 0;

	/* The smpConfig, numaConfig and members will have to be left
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

cpuTribC::~cpuTribC(void)
{
}

