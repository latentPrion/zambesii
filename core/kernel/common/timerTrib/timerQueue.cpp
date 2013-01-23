
#include <__kstdlib/__kclib/string.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/timerTrib/timerQueue.h>


timerQueueC::timerQueueC(ubit32 nativePeriod)
:
currentPeriod(nativePeriod), nativePeriod(nativePeriod), acceptingRequests(0),
device(__KNULL)
{
}

error_t timerQueueC::initialize(zkcmTimerDeviceC *device)
{
	timeS		stamp;
	error_t		ret;

	/**	CAVEAT:
	 * Don't forget to unlatch if the initialization fails.
	 **/
	if (device == __KNULL) { return ERROR_INVALID_ARG; };

	ret = device->latch(&processTrib.__kprocess.timerStream);
	if (ret != ERROR_SUCCESS)
	{
		__kprintf(WARNING TIMERQUEUE"%dns: Latch to dev \"%s\" failed."
			"\n", nativePeriod, device->getBaseDevice()->shortName);

		return ret;
	};

	stamp.seconds = 0;
	stamp.nseconds = nativePeriod;

	if (__KFLAG_TEST(
		device->capabilities.modes, ZKCM_TIMERDEV_CAP_MODE_PERIODIC))
	{
		device->setPeriodicMode(stamp);
	}
	else {
		device->setOneshotMode(stamp);
	};

	timerQueueC::device = device;
	return ERROR_SUCCESS;
}

error_t timerQueueC::enable(void)
{
	if (!isLatched()) {
		return ERROR_UNINITIALIZED;
	};

	device->enable();
	acceptingRequests = 1;
	return ERROR_SUCCESS;
}

void timerQueueC::disable(void)
{
	acceptingRequests = 0;
	/** FIXME: Should first check to see if there are objects left, and wait
	 * for them to expire before physically disabling the timer source.
	 **/
	if (!isLatched()) { return; };
	device->disable();
}


