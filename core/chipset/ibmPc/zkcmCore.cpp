
#include <chipset/zkcm/zkcmCore.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/panic.h>
#include <__kthreads/__kcpuPowerOn.h>

#include "vgaTerminal.h"


zkcmCoreC::zkcmCoreC(utf8Char *chipsetName, utf8Char *chipsetVendor)
{
	strcpy8(zkcmCoreC::chipsetName, chipsetName);
	strcpy8(zkcmCoreC::chipsetVendor, chipsetVendor);

	debug[0] = &ibmPcVgaTerminal;
	debug[1] = debug[2] = debug[3] = __KNULL;
}

error_t zkcmCoreC::initialize(void) { return ERROR_SUCCESS; }
error_t zkcmCoreC::shutdown(void) { return ERROR_SUCCESS; }
error_t zkcmCoreC::suspend(void) { return ERROR_SUCCESS; }
error_t zkcmCoreC::restore(void) { return ERROR_SUCCESS; }

void zkcmCoreC::newCpuIdNotification(cpu_t newCpuId)
{
	void		**newArray, **oldArray;

	__kcpuPowerOnSleepStacksLock.acquire();
	// If the current array is already large enough, exit.
	if ((signed)__kcpuPowerOnSleepStacksLength >= newCpuId + 1)
	{
		__kcpuPowerOnSleepStacksLock.release();
		return;
	};
	__kcpuPowerOnSleepStacksLock.release();

	// Re-size the sleepstack array.
	newArray = new void*[newCpuId + 1];
	if (newArray == __KNULL)
	{
		panic(FATAL"zkcmCore::highestCpuIdNotification: Failed to "
			"allocate sleepstack pointer array.\n");
	};

	__kcpuPowerOnSleepStacksLock.acquire();

	if (__kcpuPowerOnSleepStacksLength > 0)
	{
		memcpy(
			newArray, __kcpuPowerOnSleepStacks,
			__kcpuPowerOnSleepStacksLength * sizeof(void *));
	};

	oldArray = __kcpuPowerOnSleepStacks;
	__kcpuPowerOnSleepStacks = newArray;
	__kcpuPowerOnSleepStacksLength = newCpuId + 1;

	__kcpuPowerOnSleepStacksLock.release();

	delete oldArray;
}

zkcmCoreC		zkcmCore(CC"IBM PC compatible", CC"Unknown");

