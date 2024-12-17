
#include <debug.h>
#include <chipset/chipset.h>
#include <__kclasses/debugPipe.h>
#include <__kclasses/cachePool.h>
#include <kernel/common/debugTrib/debugTrib.h>


DebugTrib::DebugTrib(void)
{
	cpuCount.rsrc = 0;
}

void DebugTrib::enterFromIrq(void)
{
}

void DebugTrib::enterNoIrq(void)
{
	cpuCount.lock.acquire();
	cpuCount.rsrc++;
	cpuCount.lock.release();

	for (ubit16 i=0; i<DEBUGTRIB_ENTER_DELAY; i++) {};

	__kdebug.untieFrom(CHIPSET_DEBUG_DEVICE_TERMINAL_MASK);
}

void DebugTrib::exit(void)
{
	ubit16		nCpus;

	cpuCount.lock.acquire();
	nCpus = cpuCount.rsrc--;
	cpuCount.lock.release();

	if (nCpus == 0)
	{
		__kdebug.tieTo(CHIPSET_DEBUG_DEVICE_TERMINAL_MASK);
		__kdebug.refresh();
	};
}

