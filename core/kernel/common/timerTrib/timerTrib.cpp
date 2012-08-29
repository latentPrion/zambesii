
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string.h>
#include <chipset/zkcm/zkcmCore.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/timerTrib/timerTrib.h>


timerTribC::timerTribC(void)
:
period100ms(100000), period10ms(10000), period1ms(1000)
{
	watchdog.rsrc.interval = 0;
	watchdog.rsrc.isr = __KNULL;

	flags = 0;
}

error_t timerTribC::initialize(void)
{
	error_t		ret=ERROR_SUCCESS;

	// Check for the existence of a watchdog device on this chipset.
	if (zkcmCore.watchdog != __KNULL) {
		ret = (*zkcmCore.watchdog->initialize)();
	};

	// Return the result from the watchdog's initialize.
	return ret;
}

timerTribC::~timerTribC(void)
{
}

void timerTribC::dump(void)
{
	__kprintf(NOTICE"TimerTrib: dumping.\n");

	__kprintf(NOTICE"\tWatchdog: ");
		if (watchdog.rsrc.isr == __KNULL) {
			__kprintf((utf8Char *)"No.\n");
		}
		else
		{
			__kprintf((utf8Char *)"Yes: isr addr: %P, "
				"interval: %d\n",
				watchdog.rsrc.isr,
				watchdog.rsrc.interval);
		};

	__kprintf(NOTICE"\tContinuousClock: %d, %d\n",
		continuousClock.rsrc.high, continuousClock.rsrc.low);
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
			// Fixme: Handle this later.
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

status_t timerTribC::registerWatchdogIsr(zkcmIsrFn *isr, uarch_t interval)
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

