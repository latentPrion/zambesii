
#include <scaling.h>
#include <chipset/memory.h>
#include <__kstdlib/__kclib/string.h>
#include <__kthreads/__korientation.h>
#include <__kthreads/__korientationPreConstruct.h>
#include <kernel/common/task.h>
#include <kernel/common/process.h>

taskS		__korientationThread;

void __korientationPreConstruct::__korientationThreadInit(void)
{
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
}

