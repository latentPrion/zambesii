
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


timerTribC::timerTribC(void)
:
period1s(1000000000),
period100ms(100000000), period10ms(10000000), period1ms(1000000),
period100us(100000), period10us(10000), period1us(1000),
period100ns(100), period10ns(10), period1ns(1),
safePeriodMask(0), latchedPeriodMask(0),
flags(0),
clockQueueId(-1)
{
	memset(&watchdog.rsrc.interval, 0, sizeof(watchdog.rsrc.interval));
	memset(
		&watchdog.rsrc.nextFeedTime, 0,
		sizeof(watchdog.rsrc.nextFeedTime));

	watchdog.rsrc.isr = __KNULL;

	// Initialize the timer queue pointer array.
	timerQueues[0] = &period1s;
	timerQueues[1] = &period100ms;
	timerQueues[2] = &period10ms;
	timerQueues[3] = &period1ms;
	timerQueues[4] = &period100us;
	timerQueues[5] = &period10us;
	timerQueues[6] = &period1us;
	timerQueues[7] = &period100ns;
	timerQueues[8] = &period10ns;
	timerQueues[9] = &period1ns;
}

timerTribC::~timerTribC(void)
{
}

error_t timerTribC::installClockRoutine(
	ubit32 chosenTimerQueue, zkcmTimerDeviceC::clockRoutineFn *routine
	)
{
	for (ubit8 i=0; i<TIMERTRIB_TIMERQS_NQUEUES; i++)
	{
		if (!__KBIT_TEST(chosenTimerQueue, i)) { continue; };

		clockQueueId = i;
		return timerQueues[i]->installClockRoutine(routine);
	};

	return ERROR_FATAL;
}

sarch_t timerTribC::uninstallClockRoutine(void)
{
	if (clockQueueId == -1) { return 0; };

	return timerQueues[clockQueueId]->uninstallClockRoutine();
}

void timerTribC::initializeAllQueues(void)
{
	error_t			ret;
	zkcmTimerDeviceC	*dev;
	void			*handle=__KNULL;

	/**	EXPLANATION:
	 * First call initialize() on each timerQueueC object, then proceed:
	 * for each timer available from the Timer Control mod, we call the
	 * notification event function, and allow it to check whether or not
	 * the timer can be consumed by the kernel.
	 *
	 * Run a filter that will return all chipset timers indiscriminately.
	 **/
	for (uarch_t i=0; i<TIMERTRIB_TIMERQS_NQUEUES; i++)
	{
		ret = timerQueues[i]->initialize();
		if (ret != ERROR_SUCCESS)
		{
			__kprintf(ERROR TIMERTRIB"initializeAllQueues: Period "
				"%dus failed to initialize.\n",
				timerQueues[i]->getNativePeriod() / 1000);
		};
	};

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

	for (uarch_t i=0; i<TIMERTRIB_TIMERQS_NQUEUES; i++)
	{
		if (!__KBIT_TEST(safePeriodMask, i)
			|| timerQueues[i]->isLatched())
		{
			continue;
		};

		if (timerQueues[i]->testTimerDeviceSuitability(dev))
		{
			if (timerQueues[i]->latch(dev) != ERROR_SUCCESS)
			{
				__kprintf(WARNING TIMERTRIB
					"newTimerDeviceNotification: Queue %d"
					"ns failed to latch.\n",
					timerQueues[i]->getNativePeriod());

				continue;
			};

			__KBIT_SET(latchedPeriodMask, i);
			enableWaitingOnQueue(timerQueues[i]);
			return;
		};
	};

}

error_t timerTribC::enableWaitingOnQueue(ubit32 nanos)
{
	timerQueueC			*queue;

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
		queue = &period100us;
		break;

	case 10000:
		queue = &period10us;
		break;

	case 1000:
		queue = &period1us;
		break;

	case 100:
		queue = &period100ns;
		break;

	case 10:
		queue = &period10ns;
		break;

	case 1:
		queue = &period1ns;
		break;

	default:
		return ERROR_INVALID_ARG_VAL;
	};

	return enableWaitingOnQueue(queue);
}

error_t timerTribC::enableWaitingOnQueue(timerQueueC *queue)
{
	eventProcessorS::messageS	*msg;

	if (!queue->isLatched()) { return ERROR_UNINITIALIZED; };

	msg = new eventProcessorS::messageS(
		eventProcessorS::messageS::QUEUE_LATCHED,
		queue);

	if (msg == __KNULL) { return ERROR_MEMORY_NOMEM; };

	return eventProcessor.controlQueue.addItem(msg);
}

error_t timerTribC::insertTimerQueueRequestObject(timerStreamC::requestS *request)
{
	error_t		ret;
	timerQueueC	*suboptimal=__KNULL;

	for (uarch_t i=0; i<TIMERTRIB_TIMERQS_NQUEUES; i++)
	{
		// Skip queues that aren't latched to a device.
		if (!__KBIT_TEST(latchedPeriodMask, i)) { continue; };

		suboptimal = timerQueues[i];

		// Temporarily add the queue's period to the placement time.
		request->placementStamp.time.nseconds
			+= timerQueues[i]->getNativePeriod();

		/* If the object's timeout will be exceeded by placing it into
		 * this queue, skip the queue.
		 **/
		if (request->placementStamp > request->expirationStamp)
		{
			// Reset the change before continuing.
			request->placementStamp.time.nseconds
				-= timerQueues[i]->getNativePeriod();

			continue;
		};

		// Same as above: reset the change to placementStamp.
		request->placementStamp.time.nseconds
			-= timerQueues[i]->getNativePeriod();

		// Else, the request can spend at least one tick in this queue.
		timerQueues[i]->lockRequestQueue();
		ret = timerQueues[i]->insert(request);
		timerQueues[i]->unlockRequestQueue();
		return ret;
	};

	if (suboptimal == __KNULL)
	{
		__kprintf(FATAL TIMERTRIB"No timer queues are latched.\n");
		return ERROR_FATAL;
	};

	/*__kprintf(WARNING TIMERTRIB"Request placed into suboptimal queue %dus."
		"\n", suboptimal->getNativePeriod() / 1000);*/

	return suboptimal->insert(request);
}

// Called by Timer Streams to cancel Timer Request objects from Qs.
sarch_t timerTribC::cancelTimerQueueRequestObject(timerStreamC::requestS *request)
{
	sarch_t		ret;
	timerQueueC	*targetQueue;

	targetQueue = request->currentQueue;

	targetQueue->lockRequestQueue();
	ret = targetQueue->cancel(request);
	targetQueue->unlockRequestQueue();
	return ret;
}

error_t timerTribC::initialize(void)
{
	ubit8			h, m, s;
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
	zkcmCore.timerControl.refreshCachedSystemDateTime();
	/* We take the time value first because the date value is unlikely to
	 * change in the next few milliseconds.
	 **/
	zkcmCore.timerControl.getCurrentDateTime(&bootTimestamp);

	// TODO: Add %D and %T to debugPipe.printf() for date/time printing.
	h = bootTimestamp.time.seconds / 3600;
	m = (bootTimestamp.time.seconds / 60) - (h * 60);
	s = bootTimestamp.time.seconds % 60;

	__kprintf(NOTICE TIMERTRIB"Kernel boot timestamp: Date: %d-%d-%d, "
		"Time %d:%d:%d, %dus.\n",
		bootTimestamp.date.year,
		bootTimestamp.date.month,
		bootTimestamp.date.day,
		h, m, s, bootTimestamp.time.nseconds / 1000);

	// Get mask of safe periods for this chipset.
	safePeriodMask = zkcmCore.timerControl.getChipsetSafeTimerPeriods();

	// Spawn the timer event dequeueing thread.
	ret = processTrib.__kgetStream()->spawnThread(
		&timerTribC::eventProcessorS::thread, __KNULL,
		__KNULL,
		taskC::REAL_TIME,
		PRIOCLASS_CRITICAL,
		SPAWNTHREAD_FLAGS_AFFINITY_PINHERIT
		| SPAWNTHREAD_FLAGS_SCHEDPRIO_PRIOCLASS_SET
		| SPAWNTHREAD_FLAGS_DORMANT,
		&eventProcessor.task);

	if (ret != ERROR_SUCCESS) { return ret; };

	eventProcessor.tid = eventProcessor.task->getFullId();

	ret = eventProcessor.controlQueue.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	__kprintf(NOTICE TIMERTRIB"initialize: Spawned event dqer thread. "
		"addr 0x%p, id 0x%x.\n",
		eventProcessor.task, eventProcessor.tid);

	eventProcessor.controlQueue.setWaitingThread(eventProcessor.task);
	taskTrib.wake(eventProcessor.task);

	initializeAllQueues();
	zkcmCore.timerControl.timerQueuesInitializedNotification();

	// Initialize the kernel Timer Stream.
	return processTrib.__kgetStream()->timerStream.initialize();
}

void timerTribC::getCurrentTime(timeS *t)
{
	zkcmCore.timerControl.getCurrentTime(t);
}

void timerTribC::getCurrentDate(dateS *d)
{
	zkcmCore.timerControl.getCurrentDate(d);
}

void timerTribC::getCurrentDateTime(timestampS *stamp)
{
	zkcmCore.timerControl.getCurrentDateTime(stamp);
}

sarch_t timerTribC::eventProcessorS::getFreeWaitSlot(ubit8 *ret)
{
	for (*ret=0; *ret<6; *ret += 1)
	{
		if (waitSlots[*ret].eventQueue == __KNULL) {
			return 1;
		};
	};

	return 0;
}

void timerTribC::eventProcessorS::releaseWaitSlotFor(timerQueueC *timerQueue)
{
	for (ubit8 i=0; i<6; i++)
	{
		if (waitSlots[i].timerQueue == timerQueue) {
			releaseWaitSlot(i);
		};
	};
}

void timerTribC::eventProcessorS::processQueueLatchedMessage(messageS *msg)
{
	ubit8		slot;

	// Wait on the specified queue.
	if (!getFreeWaitSlot(&slot))
	{
		__kprintf(ERROR TIMERTRIB"event DQer: failed to wait on "
			"timer Q %dus: no free slots.\n",
			msg->timerQueue->getNativePeriod() / 1000);

		return;
	};

	waitSlots[slot].timerQueue = msg->timerQueue;
	waitSlots[slot].eventQueue = msg->timerQueue
		->getDevice()->getEventQueue();

	waitSlots[slot].eventQueue->setWaitingThread(
		timerTrib.eventProcessor.task);

	__kprintf(NOTICE TIMERTRIB"event DQer: Waiting on timerQueue %dus.\n\t"
		"Allocated to slot %d.\n",
		waitSlots[slot].timerQueue->getNativePeriod() / 1000, slot);
}

void timerTribC::eventProcessorS::processQueueUnlatchedMessage(messageS *msg)
{
	// Stop waiting on the specified queue.
	releaseWaitSlotFor(msg->timerQueue);
	__kprintf(NOTICE TIMERTRIB"event DQer: no longer waiting on timerQueue "
		"%dus.\n",
		msg->timerQueue->getNativePeriod() / 1000);
}

void timerTribC::eventProcessorS::processExitMessage(messageS *)
{
	__kprintf(WARNING TIMERTRIB"event DQer: Got EXIT_THREAD message.\n");
	/*UNIMPLEMENTED(
		"timerTribC::"
		"eventProcessorS::processExitMessage");*/
}

void timerTribC::sendMessage(void)
{
	eventProcessorS::messageS	*msg;

	// Posts an artificial message to the control queue.
	msg = new eventProcessorS::messageS(
		eventProcessorS::messageS::EXIT_THREAD, __KNULL);

	eventProcessor.controlQueue.addItem(msg);
}

void timerTribC::sendQMessage(void)
{
	zkcmTimerEventS		*irqEvent;

	irqEvent = period10ms.getDevice()->allocateIrqEvent();
	irqEvent->device = period10ms.getDevice();
	irqEvent->latchedStream = &processTrib.__kgetStream()->floodplainnStream;
	getCurrentTime(&irqEvent->irqStamp.time);
	getCurrentDate(&irqEvent->irqStamp.date);

	period10ms.getDevice()->getEventQueue()->addItem(irqEvent);
}

void timerTribC::eventProcessorS::thread(void *)
{
	eventProcessorS::messageS	*currMsg;
	sarch_t				messagesWereFound;
	error_t				err;

	__kprintf(NOTICE TIMERTRIB"Event DQer thread has begun executing.\n");
	for (;;)
	{
		messagesWereFound = 0;

		err = timerTrib.eventProcessor.controlQueue.pop(
			(void **)&currMsg,
			SINGLEWAITERQ_POP_FLAGS_DONTBLOCK);

		if (err == ERROR_SUCCESS)
		{
			messagesWereFound = 1;
			switch (currMsg->type)
			{
			case eventProcessorS::messageS::EXIT_THREAD:
				timerTrib.eventProcessor.processExitMessage(
					currMsg);
				break;

			case eventProcessorS::messageS::QUEUE_LATCHED:
				timerTrib.eventProcessor
					.processQueueLatchedMessage(currMsg);

				break;

			case eventProcessorS::messageS::QUEUE_UNLATCHED:
				timerTrib.eventProcessor
					.processQueueUnlatchedMessage(currMsg);

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
		zkcmTimerEventS		*currIrqEvent;
		for (ubit8 i=0; i<6; i++)
		{
			// Skip blank slots.
			if (timerTrib.eventProcessor.waitSlots[i].eventQueue
				== __KNULL)
			{
				continue;
			};

			err = timerTrib.eventProcessor.waitSlots[i].eventQueue
				->pop(
					(void **)&currIrqEvent,
					SINGLEWAITERQ_POP_FLAGS_DONTBLOCK);

			if (err == ERROR_SUCCESS)
			{
				messagesWereFound = 1;

				// Dispatch the message here.
				timerTrib.eventProcessor.waitSlots[i].timerQueue
					->tick(currIrqEvent);

				timerTrib.eventProcessor.waitSlots[i].timerQueue
					->getDevice()->freeIrqEvent(
						currIrqEvent);
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

