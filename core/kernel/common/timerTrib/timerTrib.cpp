
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/new>
#include <chipset/zkcm/zkcmCore.h>
#include <chipset/zkcm/timerDevice.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>
#include <kernel/common/timerTrib/timerTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/taskTrib/taskTrib.h>


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
period1s(1000000000),
period100ms(100000000), period10ms(10000000), period1ms(1000000),
safePeriodMask(0)
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

void timerTribC::initializeAllQueues(void)
{
	zkcmTimerDeviceC	*dev;
	void			*handle=__KNULL;

	/**	EXPLANATION:
	 * For each timer available from the Timer Control mod, we call the
	 * notification event function, and allow it to check whether or not
	 * the timer can be consumed by the kernel.
	 *
	 * Run a filter that will return all chipset timers indiscriminately.
	 **/
	dev = zkcmCore.timerControl.filterTimerDevices(
		zkcmTimerDeviceC::CHIPSET,
		0,
		(zkcmTimerDeviceC::ioLatencyE)0,
		(zkcmTimerDeviceC::precisionE)0,
		TIMERCTL_FILTER_MODE_ANY
		| TIMERCTL_FILTER_IO_ANY
		| TIMERCTL_FILTER_PREC_ANY
		| TIMERCTL_FILTER_FLAGS_SKIP_LATCHED,
		&handle);

	for (; dev != __KNULL;
		dev = zkcmCore.timerControl.filterTimerDevices(
			zkcmTimerDeviceC::CHIPSET,
			0,
			(zkcmTimerDeviceC::ioLatencyE)0,
			(zkcmTimerDeviceC::precisionE)0,
			TIMERCTL_FILTER_MODE_ANY
			| TIMERCTL_FILTER_IO_ANY
			| TIMERCTL_FILTER_PREC_ANY
			| TIMERCTL_FILTER_FLAGS_SKIP_LATCHED,
			&handle))
	{
		newTimerDeviceNotification(dev);
	};
}

void timerTribC::newTimerDeviceNotification(zkcmTimerDeviceC *dev)
{
	timeS		min, max;

	/**	EXPLANATION:
	 * Checks to see if the new timer device can be consumed by any of the
	 * kernel's timer queues.
	 **/
	dev->getPeriodicModeMinMaxPeriod(&min, &max);
	__kprintf(NOTICE TIMERTRIB"newTimerDevice: \"%s\"; min %d, max %d.\n",
		dev->getBaseDevice()->shortName, min.nseconds, max.nseconds);

	if (__KFLAG_TEST(safePeriodMask, TIMERCTL_1S_SAFE
		&& !period1s.isLatched()))
	{
		if (period1s.testTimerDeviceSuitability(dev))
		{
			period1s.initialize(dev);
			enableQueue(1000000000);
			return;
		};
	};

	if (__KFLAG_TEST(safePeriodMask, TIMERCTL_100MS_SAFE
		&& !period100ms.isLatched()))
	{
		if (period100ms.testTimerDeviceSuitability(dev))
		{
			period100ms.initialize(dev);
			enableQueue(100000000);
			return;
		};
	};

	if (__KFLAG_TEST(safePeriodMask, TIMERCTL_10MS_SAFE
		&& !period10ms.isLatched()))
	{
		if (period10ms.testTimerDeviceSuitability(dev))
		{
			period10ms.initialize(dev);
			enableQueue(10000000);
			return;
		};
	};

	if (__KFLAG_TEST(safePeriodMask, TIMERCTL_1MS_SAFE
		&& !period1ms.isLatched()))
	{
		if (period1ms.testTimerDeviceSuitability(dev))
		{
			period1ms.initialize(dev);
			enableQueue(1000000);
			return;
		};
	};
}

error_t timerTribC::enableQueue(ubit32 nanos)
{
	timerQueueC	*queue;

	switch (nanos)
	{
	case 1000000000:
		queue = &period1s;
		break;

	case 100000000:
		queue = &period100ms;
		break;

	case 10000000:
		queue = &period10ms;
		break;

	case 1000000:
		queue = &period1ms;
		break;

	case 100000:
	case 10000:
	case 1000:
	case 100:
	case 10:
	case 1:
		return ERROR_UNSUPPORTED;

	default:
		return ERROR_INVALID_ARG_VAL;
	};

	if (!queue->isLatched()) { return ERROR_UNINITIALIZED; };

	__kprintf(NOTICE TIMERTRIB"Enabling queue %dus.\n",
		queue->getNativePeriod() / 1000);

	return ERROR_SUCCESS;
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

	// Get mask of safe periods for this chipset.
	safePeriodMask = zkcmCore.timerControl.getChipsetSafeTimerPeriods();

	// Spawn the timer event dequeueing thread.
	ret = processTrib.__kprocess.spawnThread(
		(void (*)(void *))&timerTribC::eventProcessorThread, __KNULL,
		__KNULL,
		taskC::REAL_TIME,
		PRIOCLASS_CRITICAL,
		SPAWNTHREAD_FLAGS_AFFINITY_PINHERIT
		| SPAWNTHREAD_FLAGS_SCHEDPRIO_PRIOCLASS_SET
		| SPAWNTHREAD_FLAGS_DORMANT,
		&tid);

	if (ret != ERROR_SUCCESS) { return ret; };

	ret = eventProcessorControlQueue.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };
	eventProcessorTask = processTrib.__kprocess.getTask(tid);
	__kprintf(NOTICE TIMERTRIB"initialize: Spawned event dqer thread. "
		"addr 0x%p, id 0x%x.\n",
		eventProcessorTask, eventProcessorTask->id);

	eventProcessorControlQueue.setWaitingThread(eventProcessorTask);
	taskTrib.wake(eventProcessorTask);

	/*eventProcessorMessageS	*msg;
	msg = new eventProcessorMessageS;
	msg->type = eventProcessorMessageS::QUEUE_ENABLED;
	msg->timerQueue = &period10ms;
	eventProcessorControlQueue.addItem(msg);*/

	initializeAllQueues();
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
	zkcmTimerEventS		*currIrqEvent;
	ubit8			slot;
	ubit8			messagesWereFound;

	__kprintf(NOTICE TIMERTRIB"Event DQer thread has begun executing.\n");
	for (;;)
	{
		messagesWereFound = 0;

		currMsg = timerTrib.eventProcessorControlQueue.pop(
			SINGLEWAITERQ_POP_FLAGS_DONTBLOCK);

		if (currMsg != __KNULL)
		{
			messagesWereFound = 1;
			switch (currMsg->type)
			{
			case eventProcessorMessageS::EXIT_THREAD:
				// Code to exit the thread here.
				break;

			case eventProcessorMessageS::QUEUE_ENABLED:
				// Wait on the new queue.
				if (!getFreeWaitSlot(&slot))
				{
					__kprintf(ERROR TIMERTRIB"event DQer: "
						"failed to wait on "
						"timer Q %dus: no free "
						"slots.\n",
						currMsg->timerQueue
							->getNativePeriod()
							/ 1000);

					break;
				};

				currentWaitSet[slot].timerQueue =
					currMsg->timerQueue;

				currentWaitSet[slot].eventQueue =
					currMsg->timerQueue
					->getDevice()->getEventQueue();

				currentWaitSet[slot].eventQueue
					->setWaitingThread(
						timerTrib.eventProcessorTask);

				__kprintf(NOTICE TIMERTRIB"event DQer: Waiting "
					"on timerQueue %dus.\n\tAllocated to "
					"slot %d.\n",
					currentWaitSet[slot].timerQueue
						->getNativePeriod() / 1000,
					slot);

				break;

			case eventProcessorMessageS::QUEUE_DISABLED:
				// Stop waiting on this queue.
				findAndClearSlotFor(currMsg->timerQueue);
				__kprintf(NOTICE TIMERTRIB"event DQer: no "
					"longer waiting on timerQueue %dus.\n",
					currMsg->timerQueue->getNativePeriod()
						/ 1000);

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
		for (ubit8 i=0; i<6; i++)
		{
			if (currentWaitSet[i].eventQueue != __KNULL)
			{
				currIrqEvent = currentWaitSet[i].eventQueue
					->pop(
					SINGLEWAITERQ_POP_FLAGS_DONTBLOCK);

				if (currIrqEvent != __KNULL)
				{
					messagesWereFound = 1;

					// Dispatch the message here.
					currentWaitSet[i].timerQueue->tick(
						currIrqEvent);

					currentWaitSet[i].timerQueue
						->getDevice()->freeIrqEvent(
							currIrqEvent);
				};
			};
		};

		// If the loop ran to its end and there were no messages, block.
		if (!messagesWereFound) {
			taskTrib.block();
		};
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

