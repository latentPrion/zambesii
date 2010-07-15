
#include <scaling.h>
#include <__kstdlib/__kcxxlib/cstring>
#include <__kthreads/__korientationPreConstruct.h>
#include <kernel/common/task.h>
#include <kernel/common/process.h>

taskS		__korientationThread;

void __korientationPreConstruct::__korientationThreadInit(void)
{
	memset(&__korientationThread, 0, sizeof(taskS));

	__korientationThread.id = 0x1;
	__korientationThread.parent = &__kprocess;

	/* The smpConfig, numaConfig and cpuTrace members will have to be left
	 * until later since they all either are, or contain bitmapC objects.
	 *
	 * bitmapC depends on dynamic memory allocation from the numaTrib, no
	 * less. So we'll need to arrange to have a postInit() function which
	 * calls initialize on all of these items.
	 **/
}

