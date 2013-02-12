
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/new>
#include <chipset/zkcm/zkcmCore.h>
#include <chipset/zkcm/timerDevice.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>
#include <kernel/common/timerTrib/timerTrib.h>
#include <kernel/common/processTrib/processTrib.h>


/* The number of timer queues is limited and defined by the chipset's
 * safe periods. At any rate, we'll probably never exceed ~6 queues in the
 * kernel.
 **/
struct waitInfoS
{
	singleWaiterQueueC<zkcmTimerEventS>	*eventQueue;
	timerQueueC	*timerQueue;
} static currentWaitSet[6];

timerTribC::timerTribC(void)
:
period100ms(100000), period10ms(10000), period1ms(1000), safePeriodMask(0)
{
	memset(currentWaitSet, 0, sizeof(currentWaitSet));
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

void timerTribC::initializeQueue(timerQueueC *queue, ubit32 ns)
{
	void			*handle;
	zkcmTimerDeviceC	*dev;
	error_t			ret;
	timeS			min, max;

	/* EXPLANATION:
	 * We want a timer that is not already latched. We can use the timer in
	 * any mode, so that is not an issue. Precision is valued, but not of
	 * critical importance for the kernel's timer queues. I/O latency cannot
	 * exceed moderate.
	 **/
	handle = __KNULL;
	dev = zkcmCore.timerControl.filterTimerDevices(
		zkcmTimerDeviceC::CHIPSET,
		ZKCM_TIMERDEV_CAP_MODE_PERIODIC,
		zkcmTimerDeviceC::MODERATE,
		zkcmTimerDeviceC::NEGLIGABLE,
		// TIMERCTL_FILTER_SKIP_LATCHED
		0| TIMERCTL_FILTER_MODE_EXACT
		| TIMERCTL_FILTER_IO_OR_BETTER
		| TIMERCTL_FILTER_PREC_OR_BETTER,
		&handle);

	for (; dev != __KNULL;
		dev = zkcmCore.timerControl.filterTimerDevices(
			zkcmTimerDeviceC::CHIPSET,
			ZKCM_TIMERDEV_CAP_MODE_PERIODIC,
			zkcmTimerDeviceC::MODERATE,
			zkcmTimerDeviceC::NEGLIGABLE,
			//TIMERCTL_FILTER_SKIP_LATCHED
			0| TIMERCTL_FILTER_MODE_EXACT
			| TIMERCTL_FILTER_IO_OR_BETTER
			| TIMERCTL_FILTER_PREC_OR_BETTER,
			&handle))
	{
		// If the device doesn't support the period we require, skip.
		dev->getPeriodicModeMinMaxPeriod(&min, &max);
		if (ns < min.nseconds || ns > max.nseconds) {
			continue;
		};

		ret = queue->initialize(dev);
		if (ret != ERROR_SUCCESS)
		{
			__kprintf(ERROR TIMERTRIB"init Q %dns: Queue failed to "
				"initialize().\n",
				ns);

			return;
		};

		__kprintf(NOTICE TIMERTRIB"init Q %dns: Successful.\n", ns);
		return;
	};

	__kprintf(NOTICE TIMERTRIB"init Q %dns: No timer available.\n", ns);
	return;
}

error_t timerTribC::initialize(void)
{
	ubit8			h, m, s;
	processId_t		tid;
	error_t			ret;

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

	// TODO: Add %D and %T to debugPipe.printf() for date/time printing.
	h = bootTimestamp.time.seconds / 3600;
	m = (bootTimestamp.time.seconds / 60) - (h * 60);
	s = bootTimestamp.time.seconds % 60;

	__kprintf(NOTICE TIMERTRIB"Kernel boot timestamp: Date: %d-%d-%d, "
		"Time %d:%d:%d, %dns.\n",
		TIMERTRIB_DATE_GET_YEAR(bootTimestamp.date),
		TIMERTRIB_DATE_GET_MONTH(bootTimestamp.date),
		TIMERTRIB_DATE_GET_DAY(bootTimestamp.date),
		h, m, s, bootTimestamp.time.nseconds);

	// Spawn the timer event dequeueing thread.
	ret = processTrib.__kprocess.spawnThread(
		(void (*)(void *))&timerTribC::eventProcessorThread, __KNULL,
		__KNULL,
		taskC::REAL_TIME,
		PRIOCLASS_CRITICAL,
		SPAWNTHREAD_FLAGS_AFFINITY_PINHERIT
		| SPAWNTHREAD_FLAGS_SCHEDPRIO_PRIOCLASS_SET,
		&tid);

	if (ret != ERROR_SUCCESS) { return ret; };
	eventProcessorTask = processTrib.__kprocess.getTask(tid);
	__kprintf(NOTICE TIMERTRIB"initialize: Spawned event dqer thread. "
		"addr 0x%p, id 0x%x.\n",
		eventProcessorTask, eventProcessorTask->id);

	eventProcessorControlQueue.initialize(eventProcessorTask);
	taskTrib.yield();

	// Now set up the timer queues.
	safePeriodMask = zkcmCore.timerControl.getChipsetSafeTimerPeriods();
	/*if (__KFLAG_TEST(safePeriodMask, TIMERCTL_100MS_SAFE)) {
		initializeQueue(&period100ms, 100000);
	};*/

	if (__KFLAG_TEST(safePeriodMask, TIMERCTL_10MS_SAFE)) {
		initializeQueue(&period10ms, 10000);
	};

	/*if (__KFLAG_TEST(safePeriodMask, TIMERCTL_1MS_SAFE)) {
		initializeQueue(&period1ms, 1000);
	};*/

	eventProcessorMessageS		tmp;
	tmp.type = eventProcessorMessageS::QUEUE_ENABLED;
	tmp.timerQueue = &period10ms;
	eventProcessorControlQueue.addItem(&tmp);
	taskTrib.yield();

	tmp.type = eventProcessorMessageS::QUEUE_DISABLED;
	tmp.timerQueue = &period10ms;
	eventProcessorControlQueue.addItem(&tmp);
	taskTrib.yield();
	return ERROR_SUCCESS;
}

void timerTribC::getCurrentTime(timeS *t)
{
	zkcmCore.timerControl.getCurrentTime(t);
}

static sarch_t getFreeWaitSlot(ubit8 *ret)
{
	for (*ret = 0; *ret<6; *ret += 1)
	{
		if (currentWaitSet[*ret].eventQueue == __KNULL) {
			return 1;
		};
	};

	return 0;
}

static void findAndClearSlotFor(timerQueueC *timerQueue)
{
	for (ubit8 i=0; i<6; i++)
	{
		if (currentWaitSet[i].timerQueue == timerQueue)
		{
			currentWaitSet[i].timerQueue = __KNULL;
			currentWaitSet[i].eventQueue = __KNULL;
		};
	};
}

void timerTribC::eventProcessorThread(void)
{
	eventProcessorMessageS	*currMsg;
	ubit8			slot;

	for (;;)
	{
		currMsg = timerTrib.eventProcessorControlQueue.pop();
		if (currMsg != __KNULL)
		{
			switch (currMsg->type)
			{
			case eventProcessorMessageS::EXIT_THREAD:
				break;

			case eventProcessorMessageS::QUEUE_ENABLED:
				// Wait on the new queue.
				if (!getFreeWaitSlot(&slot))
				{
					__kprintf(ERROR TIMERTRIB"event DQer: "
						"failed to wait on "
						"timer Q %d ms: no free "
						"slots.\n",
						currMsg->timerQueue
							->getNativePeriod());

					break;
				};

				currentWaitSet[slot].timerQueue =
					currMsg->timerQueue;

				currentWaitSet[slot].eventQueue =
					currMsg->timerQueue
					->getDevice()->getEventQueue();

				__kprintf(NOTICE TIMERTRIB"event DQer: Waiting "
					"on timerQueue %dns.\n\tAllocated to "
					"slot %d.\n",
					currentWaitSet[slot].timerQueue
						->getNativePeriod(),
					slot);

				break;

			case eventProcessorMessageS::QUEUE_DISABLED:
				// Stop waiting on this queue.
				findAndClearSlotFor(currMsg->timerQueue);
				__kprintf(NOTICE TIMERTRIB"event DQer: no "
					"longer waiting on timerQueue %dns.\n",
					currMsg->timerQueue->getNativePeriod());

				break;

			default:
				__kprintf(NOTICE TIMERTRIB"event dqer thread: "
					"invalid message type %d.\n",
					currMsg->type);
				break;
			};

			/* Free the message and force the loop to return and
			 * check the control queue again, until it returns
			 * no new messages. Control queue messages have a higher
			 * priority than IRQ event messages.
			 **/
			delete currMsg;
			continue;
		};

		// Wait for the other queues here.
	};
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

