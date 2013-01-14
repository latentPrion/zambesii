
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/new>
#include <chipset/zkcm/zkcmCore.h>
#include <chipset/zkcm/timerDevice.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>
#include <kernel/common/timerTrib/timerTrib.h>


timerTribC::timerTribC(void)
:
period100ms(100000), period10ms(10000), period1ms(1000), safePeriodMask(0)
{
	memset(&watchdog.rsrc.interval, 0, sizeof(watchdog.rsrc.interval));
	memset(
		&watchdog.rsrc.nextFeedTime, 0,
		sizeof(watchdog.rsrc.nextFeedTime));

	watchdog.rsrc.isr = __KNULL;

	flags = 0;
}

timerTribC::~timerTribC(void)
{
}

void timerTribC::initialize100msQueue(void)
{
	void			*handle;
	zkcmTimerDeviceC	*dev;
	error_t			ret;

	/* EXPLANATION:
	 * We want a timer that is not already latched. We can use the timer in
	 * any mode, so that is not an issue. We are looking for 1s period
	 * granularity or better. Precision is not highly important for the
	 * kernel's timer queues.
	 *
	 * If we get nothing for these filter options, re-run the filter with
	 * precision being PREC_ANY and IO-latency being ANY.
	 **/
	handle = __KNULL;
	dev = zkcmCore.timerControl.filterTimerDevices(
		zkcmTimerDeviceC::CHIPSET,
		0,
		zkcmTimerDeviceC::MODERATE,
		zkcmTimerDeviceC::NEGLIGABLE,
		TIMERCTL_FILTER_SKIP_LATCHED
		| TIMERCTL_FILTER_MODE_ANY
		| TIMERCTL_FILTER_IO_OR_BETTER
		| TIMERCTL_FILTER_PREC_OR_BETTER,
		&handle);

	if (dev == __KNULL)
	{
		__kprintf(WARNING TIMERTRIB"initialize100ms: Loosening filter "
			"criteria.\n");

		handle = __KNULL;
		dev = zkcmCore.timerControl.filterTimerDevices(
			zkcmTimerDeviceC::CHIPSET,
			0,
			(zkcmTimerDeviceC::ioLatencyE)0,
			(zkcmTimerDeviceC::precisionE)0,
			TIMERCTL_FILTER_SKIP_LATCHED
			| TIMERCTL_FILTER_MODE_ANY
			| TIMERCTL_FILTER_IO_ANY
			| TIMERCTL_FILTER_PREC_ANY,
			&handle);

		if (dev == __KNULL)
		{
			/* If we can't even initialize the 1 second period
			 * queue, then we cannot continue the boot sequence.
			 * Panic.
			 **/
			__kprintf(FATAL TIMERTRIB"initialize1sQueue: Failed "
				"to get a timer for the\n"
				"\tlowest frequency queue. No respectable "
				"sources available. Halting.\n");

			panic(ERROR_RESOURCE_UNAVAILABLE);
		};
	};

	ret = period100ms.initialize(dev);
	if (ret != ERROR_SUCCESS)
	{
		__kprintf(FATAL TIMERTRIB"Failed to initialize the lowest "
			"frequency queue. Panicking.");

		panic(ret);
	};
}

void timerTribC::initialize10msQueue(void)
{
	void			*handle;
	zkcmTimerDeviceC	*dev;
	error_t			ret;

	/* EXPLANATION:
	 * See above, initialize100msQueue().
	 **/
	handle = __KNULL;
	dev = zkcmCore.timerControl.filterTimerDevices(
		zkcmTimerDeviceC::CHIPSET,
		0,
		zkcmTimerDeviceC::MODERATE,
		zkcmTimerDeviceC::NEGLIGABLE,
		TIMERCTL_FILTER_SKIP_LATCHED
		| TIMERCTL_FILTER_MODE_ANY
		| TIMERCTL_FILTER_IO_OR_BETTER
		| TIMERCTL_FILTER_PREC_OR_BETTER,
		&handle);

	if (dev == __KNULL)
	{
		handle = __KNULL;
		dev = zkcmCore.timerControl.filterTimerDevices(
			zkcmTimerDeviceC::CHIPSET,
			0,
			(zkcmTimerDeviceC::ioLatencyE)0,
			(zkcmTimerDeviceC::precisionE)0,
			TIMERCTL_FILTER_SKIP_LATCHED
			| TIMERCTL_FILTER_MODE_ANY
			| TIMERCTL_FILTER_IO_ANY
			| TIMERCTL_FILTER_PREC_ANY,
			&handle);

		if (dev == __KNULL)
		{
			// Still unable to find a respectable timer. Return.
			__kprintf(WARNING TIMERTRIB"10ms queue failed to latch "
				"onto a timer source.\n");

			return;
		};
	};

	ret = period10ms.initialize(dev);
	if (ret != ERROR_SUCCESS) {
		__kprintf(FATAL TIMERTRIB"Failed to initialize 10ms queue.\n");
	};
}

void timerTribC::initialize1msQueue(void)
{
	void			*handle;
	zkcmTimerDeviceC	*dev;
	error_t			ret;

	/* EXPLANATION:
	 * See above, initialize100msQueue().
	 **/
	handle = __KNULL;
	dev = zkcmCore.timerControl.filterTimerDevices(
		zkcmTimerDeviceC::CHIPSET,
		0,
		zkcmTimerDeviceC::MODERATE,
		zkcmTimerDeviceC::NEGLIGABLE,
		TIMERCTL_FILTER_SKIP_LATCHED
		| TIMERCTL_FILTER_MODE_ANY
		| TIMERCTL_FILTER_IO_OR_BETTER
		| TIMERCTL_FILTER_PREC_OR_BETTER,
		&handle);

	if (dev == __KNULL)
	{
		handle = __KNULL;
		dev = zkcmCore.timerControl.filterTimerDevices(
			zkcmTimerDeviceC::CHIPSET,
			0,
			(zkcmTimerDeviceC::ioLatencyE)0,
			(zkcmTimerDeviceC::precisionE)0,
			TIMERCTL_FILTER_SKIP_LATCHED
			| TIMERCTL_FILTER_MODE_ANY
			| TIMERCTL_FILTER_IO_ANY
			| TIMERCTL_FILTER_PREC_ANY,
			&handle);

		if (dev == __KNULL)
		{
			// Still unable to find a respectable timer. Return.
			__kprintf(WARNING TIMERTRIB"1ms queue failed to latch "
				"onto a timer source.\n");

			return;
		};
	};

	ret = period1ms.initialize(dev);
	if (ret != ERROR_SUCCESS) {
		__kprintf(FATAL TIMERTRIB"Failed to initialize 1ms queue.\n");
	};
}

error_t timerTribC::initialize(void)
{
	ubit8			h, m, s;

	/**	EXPLANATION:
	 * Fills out the boot timestamp and sets up the Timer Tributary's Timer
	 * queues.
	 */

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
	zkcmCore.timerControl.refreshCachedSystemTime();
	/* We take the time value first because the date value is unlikely to
	 * change in the next few milliseconds.
	 **/
	zkcmCore.timerControl.getCurrentTime(&bootTimestamp.time);
	zkcmCore.timerControl.getCurrentDate(&bootTimestamp.date);

	h = bootTimestamp.time.seconds / 3600;
	m = (bootTimestamp.time.seconds / 60) - (h * 60);
	s = bootTimestamp.time.seconds % 60;

	__kprintf(NOTICE TIMERTRIB"Kernel boot timestamp: Date: %d-%d-%d, "
		"Time %d:%d:%d, %dns.\n",
		TIMERTRIB_DATE_GET_YEAR(bootTimestamp.date),
		TIMERTRIB_DATE_GET_MONTH(bootTimestamp.date),
		TIMERTRIB_DATE_GET_DAY(bootTimestamp.date),
		h, m, s, bootTimestamp.time.nseconds);

	// Now set up the timer queues.
	safePeriodMask = zkcmCore.timerControl.getChipsetSafeTimerPeriods();
	if (__KFLAG_TEST(safePeriodMask, TIMERCTL_100MS_SAFE)) {
__kprintf(NOTICE TIMERTRIB"initialize(): about to init 100ms queue.\n");
		initialize100msQueue();
	};

	/*if (__KFLAG_TEST(safePeriodMask, TIMERCTL_10MS_SAFE)) {
		initialize10msQueue();
	};

	if (__KFLAG_TEST(safePeriodMask, TIMERCTL_1MS_SAFE)) {
		initialize1msQueue();
	};*/

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
	if (isr == __KNULL) { return ERROR_INVALID_ARG; };
	if ((interval.seconds == 0) && (interval.nseconds == 0)) {
		return ERROR_INVALID_ARG_VAL;
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

#if 0
void timerTribC::timerDeviceTimeoutEvent(zkcmTimerDeviceC *dev)
{
	timerStreamC		*stream;
	zkcmTimerEventS		*event;

	/**	EXPLANATION:
	 * Called from IRQ context by a timer device driver to notify the kernel
	 * that a timer device's IRQ has occured. This function does the
	 * following:
	 *	1. Gets the ID of the process which is latched to the calling
	 *	   timer device.
	 *	2. Gets a handle to that process's Timer Stream object.
	 *	3. Allocates a new zkcmTimerEventS structure.
	 *	4. Fills it out with pertinent information about the timed out
	 *	   timer event that has just been signaled.
	 *	5. Places it into the queue of timer event notifications for
	 *	   the target process' Timer Stream object.
	 *
	 * At this point, the kernel returns control to the IRQ handler routine,
	 * which will generally acknowledge the IRQ to the device (if it hadn't
	 * already done so before notifying the kernel) and exit.
	 **/
	event = new zkcmTimerEventS;
	if (event == __KNULL)
	{
		__kprintf(ERROR TIMERTRIB"timerDeviceTimeoutEvent: device %s:\n"
			"\tFailed to alloc timeout object.\n",
			dev->getBaseDevice()->shortName);

		return;
	};

	event->device = dev;
	dev->getLatchState(&event->latchedStream);
	getCurrentTime(&event->irqTime);

	//event->latchedStream->timerDeviceTimeoutNotification(event);
}
#endif

