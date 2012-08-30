
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string.h>
#include <chipset/zkcm/zkcmCore.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/timerTrib/timerTrib.h>


timerTribC::timerTribC(void)
:
period100ms(100000), period10ms(10000), period1ms(1000)
{
	memset(&watchdog.rsrc.interval, 0, sizeof(watchdog.rsrc.interval));
	memset(
		&watchdog.rsrc.nextFeedTime, 0,
		sizeof(watchdog.rsrc.nextFeedTime));

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
			__kprintf((utf8Char *)"Yes: isr addr: 0x%p, "
				"interval: %ds,%dns\n",
				watchdog.rsrc.isr,
				watchdog.rsrc.interval.seconds,
				watchdog.rsrc.interval.nseconds);
		};
}

status_t timerTribC::registerWatchdogIsr(zkcmIsrFn *isr, timeS interval)
{
	if ((isr == __KNULL)
		|| ((interval.seconds == 0) && (interval.nseconds == 0)))
	{
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

		memcpy(&watchdog.rsrc.interval, &interval, sizeof(interval));
		// chipset_getDate(&watchdog.rsrc.nextFeedTime.date);
		// chipset_getTime(&watchdog.rsrc.nextFeedTime.time);
		// watchdog.rsrc.nextFeedTime.time.seconds
		//	+= watchdog.rsrc.interval.seconds;
		// watchdog.rsrc.nextFeedTime.time.nseconds
		//	+= watchdog.rsrc.interval.nseconds;

		watchdog.lock.release();

		return ERROR_SUCCESS;
	};
}

void timerTribC::updateWatchdogInterval(timeS interval)
{
	if (interval.seconds == 0 && interval.nseconds == 0) { return; };

	watchdog.lock.acquire();
	memcpy(&watchdog.rsrc.interval, &interval, sizeof(interval));
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

		memset(
			&watchdog.rsrc.interval, 0,
			sizeof(watchdog.rsrc.interval));

		memset(
			&watchdog.rsrc.nextFeedTime, 0,
			sizeof(watchdog.rsrc.nextFeedTime));

		watchdog.lock.release();
		return;
	};
}

