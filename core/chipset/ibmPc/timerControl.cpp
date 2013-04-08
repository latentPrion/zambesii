
#include <chipset/zkcm/timerControl.h>
#include <chipset/zkcm/timerDevice.h>
#include <chipset/zkcm/timerControl_filters.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kbitManipulation.h>
#include <__kclasses/ptrList.h>
#include <kernel/common/panic.h>
#include <kernel/common/timerTrib/timerTrib.h>

#include "rtcmos.h"
#include "i8254.h"


#define IBMPC_TIMERCTL		"Timer Control: "

static ptrListC<zkcmTimerDeviceC>	timers;
static sharedResourceGroupC<multipleReaderLockC, timestampS>	systemTime;
static const ubit32			ibmPcSafePeriodMask =
	/*TIMERCTL_1S_SAFE
	|*/ TIMERCTL_100MS_SAFE | TIMERCTL_10MS_SAFE /*| TIMERCTL_1MS_SAFE*/;

static const ubit8 daysInMonth[12] =
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

static void updateSystemTime(ubit32 tickGranularity)
{
	systemTime.lock.writeAcquire();
	systemTime.rsrc.time.nseconds += tickGranularity;
	// If nanoseconds overflow...
	if (systemTime.rsrc.time.nseconds >= 1000000000)
	{
		systemTime.rsrc.time.nseconds -= 1000000000;
		systemTime.rsrc.time.seconds++;
		// If seconds overflow...
		if (systemTime.rsrc.time.seconds >= 86400)
		{
			systemTime.rsrc.time.seconds -= 86400;
			systemTime.rsrc.date.day++;
			systemTime.rsrc.date.weekDay++;
			if (systemTime.rsrc.date.weekDay > 7) {
				systemTime.rsrc.date.weekDay -= 7;
			};

			if (systemTime.rsrc.date.day
				> daysInMonth[systemTime.rsrc.date.month])
			{
				systemTime.rsrc.date.day = 1;
				systemTime.rsrc.date.month++;
				if (systemTime.rsrc.date.month > 12)
				{
					systemTime.rsrc.date.month = 1;
					systemTime.rsrc.date.year++;
				};
			};
		};
	};
	systemTime.lock.writeRelease();
}

status_t zkcmTimerControlModC::getCurrentDate(dateS *date)
{
	uarch_t		rwFlags;

	systemTime.lock.readAcquire(&rwFlags);
	*date = systemTime.rsrc.date;
	systemTime.lock.readRelease(rwFlags);

	return ERROR_SUCCESS;
}

status_t zkcmTimerControlModC::getCurrentTime(timeS *time)
{
	uarch_t		rwFlags;

	systemTime.lock.readAcquire(&rwFlags);
	*time = systemTime.rsrc.time;
	systemTime.lock.readRelease(rwFlags);

	return ERROR_SUCCESS;
}

status_t zkcmTimerControlModC::getCurrentDateTime(timestampS *stamp)
{
	uarch_t		rwFlags;

	systemTime.lock.readAcquire(&rwFlags);
	*stamp = systemTime.rsrc;
	systemTime.lock.readRelease(rwFlags);

	return ERROR_SUCCESS;
}

void zkcmTimerControlModC::flushCachedSystemDateTime(void)
{
	UNIMPLEMENTED("zkcmTimerControlModC::flushCachedSystemTime()");
}

void zkcmTimerControlModC::refreshCachedSystemDateTime(void)
{
	systemTime.lock.writeAcquire();

	ibmPc_rtc_getHardwareTime(&systemTime.rsrc.time);
	ibmPc_rtc_getHardwareDate(&systemTime.rsrc.date);

	systemTime.lock.writeRelease();
}

void zkcmTimerControlModC::timerQueuesInitializedNotification(void)
{
	ubit32		latchedQueueMask;

	latchedQueueMask = timerTrib.getLatchedTimerQueueMask();

	/**	EXPLANATION:
	 * IBM PC would prefer to have the system clock updated with 1ms
	 * granularity, but most of the time it will only be able to get
	 * 10ms granularity; at least until after ACPICA has been loaded.
	 **/

	/* Array bounds hardcoded, but this basically means we only check for
	 * periods 100ms, 10ms and 1ms.
	 **/
	for (ubit32 i=3; i>0; i--)
	{
		if (!__KBIT_TEST(latchedQueueMask, i)) { continue; };

		timerTrib.installClockRoutine((1<<i), &updateSystemTime);
		__kprintf(NOTICE IBMPC_TIMERCTL"System timekeeper routine "
			"installed on timer queue %d.\n",
			i);

		return;
	};

	panic(CC"System timekeeping is not available within reason on "
		"this chipset.\n");
}

error_t zkcmTimerControlModC::initialize(void)
{
	error_t		ret;

	ret = timers.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	ret = ibmPc_rtc_initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	return i8254Pit.initialize();
}

error_t zkcmTimerControlModC::shutdown(void)
{
	return ERROR_SUCCESS;
}

error_t zkcmTimerControlModC::suspend(void)
{
	return ERROR_SUCCESS;
}

error_t zkcmTimerControlModC::restore(void)
{
	return ERROR_SUCCESS;
}

ubit32 zkcmTimerControlModC::getChipsetSafeTimerPeriods(void)
{
	/** EXPLANATION:
	 * For IBM-PC, safe timer periods are 1s, 100ms, 10ms and 1ms. The
	 * kernel won't support nanosecond resolution on the IBM-PC until the
	 * industry has securely moved in that direction.
	 **/
	return ibmPcSafePeriodMask;
}

zkcmTimerDeviceC *zkcmTimerControlModC::filterTimerDevices(
	zkcmTimerDeviceC::timerTypeE type,	// PER_CPU or CHIPSET.
	ubit32 modes,				// PERIODIC | ONESHOT.
	zkcmTimerDeviceC::ioLatencyE ioLatency,	// LOW, MODERATE or HIGH
	zkcmTimerDeviceC::precisionE precision,	// EXACT, NEGLIGABLE,
						// OVERFLOW or UNDERFLOW
	ubit32 flags,
	void **handle
	)
{
	zkcmTimerDeviceC	*source;
	void			*owner;

	for (source = timers.getNextItem(handle);
		source != __KNULL;
		source = timers.getNextItem(handle))
	{
		// Must meet all of the criteria passed to us.
		if (__KFLAG_TEST(flags, TIMERCTL_FILTER_FLAGS_SKIP_LATCHED))
		{
			if (source->getLatchState((floodplainnStreamC **)&owner)) {
				continue;
			};
		};

		if (source->capabilities.type != type) { continue; };
		if (!timerFilters::modes(source, modes, flags)) { continue; };
		if (!timerFilters::ioLatency(source, ioLatency, flags)) {
			continue;
		};

		if (!timerFilters::precision(source, precision, flags)) {
			continue;
		};

		return source;
	};

	return __KNULL;
}

static utf8Char *timerDevTypes[] = { CC"Per-cpu", CC"chipset" };
static utf8Char *timerDevModes[] =
{
	CC"None (?)", CC"Periodic", CC"Oneshot", CC"Periodic and oneshot"
};

static utf8Char *timerDevPrecisions[] =
{
	CC"Exact", CC"Negligable", CC"Overflow", CC"Underflow", CC"Any"
};

static utf8Char *timerDevIoLatencies[] = { CC"Low", CC"Moderate", CC"High" };

static void dumpTimerDeviceInfo(zkcmTimerDeviceC *dev)
{
	__kprintf(NOTICE"ZKCM Timer Device: (childId %d) \"%s\"\n"
		"\t%s\n"
		"\tType: %s; supported modes %s.\n"
		"\tPrecision %s, ioLatency %s.\n",
		dev->getBaseDevice()->childId, dev->getBaseDevice()->shortName,
		dev->getBaseDevice()->longName,
		timerDevTypes[dev->capabilities.type],
		timerDevModes[dev->capabilities.modes],
		timerDevPrecisions[dev->capabilities.precision],
		timerDevIoLatencies[dev->capabilities.ioLatency]);
}	

error_t zkcmTimerControlModC::registerNewTimerDevice(zkcmTimerDeviceC *timer)
{
	error_t		ret;

	// Ensure it's not already been added, then add it to the list.
	if (!timers.checkForItem(timer))
	{
		ret = timers.insert(timer);
		if (ret != ERROR_SUCCESS)
		{
			__kprintf(ERROR IBMPC_TIMERCTL"registerNewTimerDevice: "
				"Failed to add timer:\n");

			dumpTimerDeviceInfo(timer);
		};

		__kprintf(NOTICE IBMPC_TIMERCTL"registerNewTimerDevice: Added "
			"timer:\n");

		dumpTimerDeviceInfo(timer);
		return ERROR_SUCCESS;
	};

	return ERROR_INVALID_ARG_VAL;
}

error_t zkcmTimerControlModC::unregisterTimerDevice(
	zkcmTimerDeviceC *timer, uarch_t flags
	)
{
	void	**latchedStream;

	if (!__KFLAG_TEST(flags, TIMERCTL_UNREGISTER_FLAGS_FORCE))
	{
		if (timer->getLatchState((floodplainnStreamC **)&latchedStream))
		{
			__kprintf(ERROR IBMPC_TIMERCTL"unregisterTimerDevice: "
				"Device is latched.\n");

			return ERROR_RESOURCE_BUSY;
		};
	};

	timers.remove(timer);
	return ERROR_SUCCESS;
}

