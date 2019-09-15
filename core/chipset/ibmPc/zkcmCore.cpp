
#include <chipset/zkcm/zkcmCore.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/panic.h>
#include <__kthreads/__kcpuPowerOn.h>

#include "vgaTerminal.h"
#include "rs232.h"


ZkcmCore		zkcmCore(CC"IBM PC compatible", CC"Unknown");

ZkcmCore::ZkcmCore(utf8Char *chipsetName, utf8Char *chipsetVendor)
{
	strcpy8(ZkcmCore::chipsetName, chipsetName);
	strcpy8(ZkcmCore::chipsetVendor, chipsetVendor);

	debug[0] = &ibmPcVgaTerminal;
	debug[1] = &ibmPcRs2320;
	debug[2] = debug[3] = NULL;
}

error_t ZkcmCore::initialize(void) { return ERROR_SUCCESS; }
error_t ZkcmCore::shutdown(void) { return ERROR_SUCCESS; }
error_t ZkcmCore::suspend(void) { return ERROR_SUCCESS; }
error_t ZkcmCore::restore(void) { return ERROR_SUCCESS; }

void ZkcmCore::newCpuIdNotification(cpu_t newCpuId)
{
	void		**newArray, **oldArray;

	__kcpuPowerStacksLock.acquire();
	// If the current array is already large enough, exit.
	if (__kcpuPowerStacksLength >= (unsigned)newCpuId + 1)
	{
		__kcpuPowerStacksLock.release();
		return;
	};
	__kcpuPowerStacksLock.release();

	// Re-size the sleepstack array.
	newArray = new void*[newCpuId + 1];
	if (newArray == NULL)
	{
		panic(FATAL"zkcmCore::newCpuIdNotification: Failed to "
			"allocate sleepstack pointer array.\n");
	};

	__kcpuPowerStacksLock.acquire();

	if (__kcpuPowerStacksLength > 0)
	{
		memcpy(
			newArray, __kcpuPowerStacks,
			__kcpuPowerStacksLength * sizeof(void *));
	};

	oldArray = __kcpuPowerStacks;
	__kcpuPowerStacks = newArray;
	__kcpuPowerStacksLength = newCpuId + 1;

	__kcpuPowerStacksLock.release();

	delete oldArray;
}

void ZkcmCore::chipsetEventNotification(e__kPowerEvent event, uarch_t flags)
{
	/* We could unconditionally call the submodules with every event, but
	 * we deliberately use a switch to ensure that only those events that
	 * are known to be handled by particular modules are passed to those
	 * modules, and that we can panic() inside of a module if an event that
	 * is unknown to it is passed to it.
	 */
	switch (event)
	{
	case __KPOWER_EVENT___KMEMORY_STREAM_AVAIL:
		ibmPcVgaTerminal.chipsetEventNotification(event, flags);
		break;

	case __KPOWER_EVENT_HEAP_AVAIL:
	case __KPOWER_EVENT_PRE_SMP_MODE_SWITCH:
		irqControl.chipsetEventNotification(event, flags);
		break;

	default:
		printf(WARNING ZKCMCORE"Unhandled event %d.\n", event);
		return;
	}
}
