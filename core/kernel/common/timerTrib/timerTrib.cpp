
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string.h>
#include <kernel/common/timerTrib/timerTrib.h>
#include <kernel/common/moduleApis/chipsetSupportPackage.h>

timerTribC::timerTribC(void)
{
}

error_t timerTribC::initialize(void)
{
	error_t		ret=ERROR_SUCCESS;

	watchdog.rsrc.isr = watchdog.rsrc.interval = 0;

	// Check for the existence of a watchdog device on this chipset.
	if (chipsetCoreDev.watchdogDev != __KNULL) {
		ret = (*chipsetCoreDev.watchdogDev->initialize)();
	};

	// Return the result from the watchdog's initialize.
	return ret;
}

timerTribC::~timerTribC(void)
{
}

void timerTribC::updateContinuousClock(void)
{
	// Update global continuous clock.
	continuousClock.lock.acquire();
	continuousClock.rsrc++;
	continuousClock.lock.release();

	watchdog.lock.acquire();

	if (watchdog.rsrc.isr == __KNULL) {
		watchdog.lock.release();
	}
	else
	{
		continuousClock.lock.acquire();
		if (continuousClock.rsrc == watchdog.rsrc.feedTime)
		{
			continuousClock.lock.release();

			watchdog.rsrc.feedTime += watchdog.rsrc.interval;

			watchdog.lock.release();
			(*watchdog.rsrc.isr)();

			goto skipWatchdogLockRelease;
		}
		else {
			continuousClock.lock.release();
		};
		watchdog.lock.release();
skipWatchdogLockRelease:;
	};		
}

status_t timerTribC::registerWatchdogIsr(status_t (*isr)(), uarch_t interval)
{
	if ((isr == __KNULL) || (interval == 0)) {
		return ERROR_INVALID_ARG;
	};

	watchdog.lock.acquire();

	if (watchdog.rsrc.isr != __KNULL)
	{
		watchdog.lock.release();
		return TIMERTRIB_WATCHDOG_ALREADY_REGISTERED;
	}
	else
	{
		watchdog.rsrc.isr = isr;
		watchdog.rsrc.interval = interval;

		continuousClock.lock.acquire();
		watchdog.rsrc.feedTime = continuousClock.rsrc;
		watchdog.rsrc.feedTime += interval;

		continuousClock.lock.release();
		watchdog.lock.release();

		return ERROR_SUCCESS;
	};
}

void timerTribC::updateWatchdogIsr(uarch_t interval)
{
	if (interval == 0) {
		return;
	};

	watchdog.lock.acquire();
	watchdog.rsrc.interval = interval;
	watchdog.lock.release();
}

void timerTribC::unregisterWatchdogIsr(void)
{
	watchdog.lock.acquire();

	if (watchdog.rsrc.isr == __KNULL)
	{
		watchdog.lock.release();
		return;
	}
	else
	{
		watchdog.rsrc.isr = __KNULL;
		watchdog.rsrc.interval = 0;
		watchdog.rsrc.feedTime = clock_t(0, 0);

		watchdog.lock.release();
		return;
	};
}

void timerTribC::enablePerCpuClockEmu(void)
{
	__KFLAG_SET(flags, TIMERTRIB_PER_CPU_CLOCK_EMU);
}

void timerTribC::disablePerCpuClockEmu(void)
{
	__KFLAG_UNSET(flags, TIMERTRIB_PER_CPU_CLOCK_EMU);
}

