
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

error_t timerTribC::initialize2(void)
{
	error_t			ret;
	zkcmTimerControlModS	*timerControl;
	ubit8			h, m, s;

	/**	EXPLANATION:
	 * Initializes the chipset's Timer Control ZKCM module, fills out the
	 * boot timestamp, and sets up the Timer Tributary's Timer queues.
	 **/
	timerControl = zkcmCore.timerControl;
	ret = (*timerControl->initialize)();
	if (ret != ERROR_SUCCESS) {
		return ret;
	};

	/* For chipsets which have accurate time keeping hardware, the following
	 * step is not necessary and those chipsets will generally do nothing.
	 *
	 * However, for chipsets where reading the hardware clock is very
	 * expensive, or where the hardware clock's accuracy is unreliable, the
	 * chipset may need to set up a timer source and do manual time keeping
	 * using a cached time value in RAM. For these chipsets, reading the
	 * current date/time right now /may/ yield a blank timestamp, depending
	 * on how they go about setting up their manual timer source.
	 *
	 * For that reason, we use refreshCachedSystemTime() to force such a
	 * chipset to update the value they return to us with the correct time
	 * value from the hardware clock, so we can get a relatively accurate
	 * boot timestamp value.
	 **/
	(*timerControl->refreshCachedSystemTime)();
	/* We take the time value first because the date value is unlikely to
	 * change in the next few milliseconds.
	 **/
	(*timerControl->getCurrentTime)(&bootTimestamp.time);
	(*timerControl->getCurrentDate)(&bootTimestamp.date);

	h = bootTimestamp.time.seconds / 3600;
	m = (bootTimestamp.time.seconds / 60) - (h * 60);
	s = bootTimestamp.time.seconds % 60;

	__kprintf(NOTICE TIMERTRIB"Kernel boot timestamp: Date: %d-%d-%d,\n"
		"\tTime %d:%d:%d, %dns.\n",
		TIMERTRIB_DATE_GET_YEAR(bootTimestamp.date),
		TIMERTRIB_DATE_GET_MONTH(bootTimestamp.date),
		TIMERTRIB_DATE_GET_DAY(bootTimestamp.date),
		h, m, s, bootTimestamp.time.nseconds);

	return ERROR_SUCCESS;
}

void timerTribC::dump(void)
{
	__kprintf(NOTICE TIMERTRIB"Dumping.\n");

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

