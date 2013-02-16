
#include <__kstdlib/__kclib/string.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/timerTrib/timerQueue.h>


/**	EXPLANATION:
 * Timer Queue is the class used to represent queues of sorted timer request
 * objects from processes (kernel, driver and user). At boot, the kernel will
 * check to see how many timer sources the chipset provides, and how many of
 * them can be used to initialize timer queues. Subsequently, if any new
 * timer queues are detected, the kernel will automatically attempt to see if
 * it can be used to initialize any still-uninitialized timer queues.
 *
 * Any left-over timer sources are exposed to any process which may want to use
 * them via the ZKCM Timer Control mod's TimerFilter api.
 *
 * The number of Timer Queues that the kernel initializes for a chipset is
 * of course, chipset dependent. It is based on the chipset's "safe period bmp",
 * which states what timer periods the chipset can handle safely. Not every
 * chipset can have a timer running at 100Hz and remain stable, for example.
 **/

timerQueueC::timerQueueC(ubit32 nativePeriod)
:
currentPeriod(nativePeriod), nativePeriod(nativePeriod), acceptingRequests(0),
device(__KNULL)
{
}

error_t timerQueueC::initialize(zkcmTimerDeviceC *device)
{
	error_t		ret;

	/**	CAVEAT:
	 * Don't forget to unlatch if the initialization fails.
	 **/
	if (device == __KNULL) { return ERROR_INVALID_ARG; };

	ret = device->latch(&processTrib.__kprocess.floodplainnStream);
	if (ret != ERROR_SUCCESS)
	{
		__kprintf(WARNING TIMERQUEUE"%dns: Latch to dev \"%s\" failed."
			"\n", nativePeriod, device->getBaseDevice()->shortName);

		return ret;
	};

	timerQueueC::device = device;
	return ERROR_SUCCESS;
}

error_t timerQueueC::enable(void)
{
	error_t		ret;
	timeS		stamp;

	if (!isLatched()) {
		return ERROR_UNINITIALIZED;
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

	ret = device->enable();
	if (ret != ERROR_SUCCESS)
	{
		__kprintf(ERROR TIMERQUEUE"%dns: Failed to enable() device.\n",
			getNativePeriod());

		return ret;
	};

	return ERROR_SUCCESS;
}

void timerQueueC::disable(void)
{
	/** FIXME: Should first check to see if there are objects left, and wait
	 * for them to expire before physically disabling the timer source.
	 **/
	if (!isLatched()) { return; };
	device->disable();
}

void timerQueueC::tick(zkcmTimerEventS *)
{
	__kprintf(NOTICE TIMERQUEUE"%dns: Tick!\n", getNativePeriod());
}

