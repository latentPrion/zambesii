
#include <debug.h>
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
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/zasyncStream.h>


TimerTrib::TimerTrib(void)
:
period1s(1000000000),
period100ms(100000000), period10ms(10000000), period1ms(1000000),
period100us(100000), period10us(10000), period1us(1000),
period100ns(100), period10ns(10), period1ns(1),
safePeriodMask(0), latchedPeriodMask(0),
flags(0),
clockQueueId(-1),
watchdog(CC"TimerTrib watchdog")
{
	memset(&watchdog.rsrc.interval, 0, sizeof(watchdog.rsrc.interval));
	memset(
		&watchdog.rsrc.nextFeedTime, 0,
		sizeof(watchdog.rsrc.nextFeedTime));

	watchdog.rsrc.isr = NULL;

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

TimerTrib::~TimerTrib(void)
{
}

error_t TimerTrib::installClockRoutine(
	ubit32 chosenTimerQueue, ZkcmTimerDevice::clockRoutineFn *routine
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

sarch_t TimerTrib::uninstallClockRoutine(void)
{
	if (clockQueueId == -1) { return 0; };

	return timerQueues[clockQueueId]->uninstallClockRoutine();
}

void TimerTrib::initializeAllQueuesReq(void)
{
	error_t			ret;
	ZkcmTimerDevice		*dev;

	/**	EXPLANATION:
	 * First call initialize() on each TimerQueue object, then proceed:
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
			printf(ERROR TIMERTRIB"initializeAllQueues: Period "
				"%dus failed to initialize.\n",
				timerQueues[i]->getNativePeriod() / 1000);
		};
	};

	HeapList<ZkcmTimerDevice>::Iterator		it;

	dev = zkcmCore.timerControl.filterTimerDevices(
		ZkcmTimerDevice::CHIPSET,
		0,
		(ZkcmTimerDevice::ioLatencyE)0,
		(ZkcmTimerDevice::precisionE)0,
		TIMERCTL_FILTER_MODE_ANY
		| TIMERCTL_FILTER_IO_ANY
		| TIMERCTL_FILTER_PREC_ANY
		| TIMERCTL_FILTER_FLAGS_SKIP_LATCHED,
		&it);

	for (; dev != NULL;
		dev = zkcmCore.timerControl.filterTimerDevices(
			ZkcmTimerDevice::CHIPSET,
			0,
			(ZkcmTimerDevice::ioLatencyE)0,
			(ZkcmTimerDevice::precisionE)0,
			TIMERCTL_FILTER_MODE_ANY
			| TIMERCTL_FILTER_IO_ANY
			| TIMERCTL_FILTER_PREC_ANY
			| TIMERCTL_FILTER_FLAGS_SKIP_LATCHED,
			&it))
	{
		newTimerDeviceInd(dev);
	};
}

void TimerTrib::newTimerDeviceInd(ZkcmTimerDevice *dev)
{
	sTime		min, max;
	ubit8		dummySym;

	/**	EXPLANATION:
	 * Checks to see if the new timer device can be consumed by any of the
	 * kernel's timer queues.
	 **/
	dev->getPeriodicModeMinMaxPeriod(&min, &max);
	printf(NOTICE TIMERTRIB"newTimerDevice: \"%s\"; min %d, max %d.\n",
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
				printf(WARNING TIMERTRIB
					"newTimerDeviceNotification: Queue %d"
					"ns failed to latch.\n",
					timerQueues[i]->getNativePeriod());

				continue;
			};

			__KBIT_SET(latchedPeriodMask, i);
			eventProcessor.enableWaitingOnQueue(
				timerQueues[i], 1,
				reinterpret_cast<MessageStreamCb *>(&dummySym));

			return;
		};
	};

}

TimerQueue* TimerTrib::lookupQueueForPeriodNs(ubit32 nanos)
{
	switch (nanos)
	{
	case 1000000000:
		return &period1s;

	case 100000000:
		return &period100ms;

	case 10000000:
		return &period10ms;

	case 1000000:
		return &period1ms;

	case 100000:
		return &period100us;

	case 10000:
		return &period10us;

	case 1000:
		return &period1us;

	case 100:
		return &period100ns;

	case 10:
		return &period10ns;

	case 1:
		return &period1ns;

	default:
		return NULL;
	}
}

error_t TimerTrib::enableWaitingOnQueue(ubit32 nanos)
{
	ubit8			dummySym;
	TimerQueue *queue = lookupQueueForPeriodNs(nanos);

	if (queue == NULL) { return ERROR_INVALID_ARG_VAL; };

	return eventProcessor.enableWaitingOnQueue(
		queue, 1, reinterpret_cast<MessageStreamCb*>(&dummySym));
}

error_t TimerTrib::disableWaitingOnQueue(ubit32 nanos)
{
	ubit8			dummySym;
	TimerQueue *queue = lookupQueueForPeriodNs(nanos);

	if (queue == NULL) { return ERROR_INVALID_ARG_VAL; };

	return eventProcessor.disableWaitingOnQueue(
		queue, 1, reinterpret_cast<MessageStreamCb*>(&dummySym));
}

error_t TimerTrib::initialize(void)
{
	ubit8			h, m, s, dummySym;
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

	printf(NOTICE TIMERTRIB"Kernel boot timestamp: Date: %d-%d-%d, "
		"Time %d:%d:%d, %dus.\n",
		bootTimestamp.date.year,
		bootTimestamp.date.month,
		bootTimestamp.date.day,
		h, m, s, bootTimestamp.time.nseconds / 1000);

	// Get mask of safe periods for this chipset.
	safePeriodMask = zkcmCore.timerControl.getChipsetSafeTimerPeriods();

	/* Spawn the timer event dequeueing thread. Use the dummySym variable
	 * to identify the ACK msg.
	 **/
	ret = processTrib.__kgetStream()->spawnThread(
		&TimerTrib::sEventProcessor::thread, NULL,
		NULL,
		Thread::REAL_TIME,
		PRIOCLASS_CRITICAL,
		SPAWNTHREAD_FLAGS_AFFINITY_PINHERIT
		| SPAWNTHREAD_FLAGS_SCHEDPRIO_PRIOCLASS_SET
		| SPAWNTHREAD_FLAGS_DORMANT,
		&eventProcessor.task, &dummySym);

	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR TIMERTRIB"Failed to spawn event dqer thread: "
			"%d.\n", ret);
		return ret;
	}

	eventProcessor.tid = eventProcessor.task->getFullId();

	printf(NOTICE TIMERTRIB"initialize: Spawned event dqer thread. "
		"addr %p, id %x.\n",
		eventProcessor.task, eventProcessor.tid);

printf(FATAL TIMERTRIB"initialize: DQer thread ID is %x.\n",
	eventProcessor.task->getFullId());
	/** EXPLANATION:
	 * We spawned the event processor thread with the DORMANT flag
	 * set, so it won't run until we wake it. This enabled us to
	 * do some initialization here without worrying about race
	 * conditions with the event processor thread.
	 *
	 * wake() will cause the event processor thread to wake up and
	 * initialize. When it's initialized, it'll send an ACK msg back
	 * to us, which will cause us to resume execution at
	 * initialize_contd2() below.
	 **/
	if (taskTrib.schedule(eventProcessor.task) != ERROR_SUCCESS)
	{
		printf(ERROR TIMERTRIB"Failed to schedule event processor "
			"thread!\n");
		return ERROR_UNKNOWN;
	}
	if (taskTrib.wake(eventProcessor.task) != ERROR_SUCCESS)
	{
		printf(ERROR TIMERTRIB"Failed to wake event processor "
			"thread!\n");
		return ERROR_UNKNOWN;
	}

	printf(FATAL TIMERTRIB"initialize: wake(%x) returned %d.\n",
		eventProcessor.task->getFullId(), ret);

	MessageStream::sHeader		*threadAckMsg=NULL;
	MessageStream::Filter		filter(
		0, 0, 0, 0, 0, &dummySym,
		MessageStream::Filter::FLAG_PRIVATE_DATA);

	// Block until the event processor thread sends an ACK msg.
	ret = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread()
		->messageStream.pull(
			&threadAckMsg, 0, &filter);

	if (ret != ERROR_SUCCESS || threadAckMsg->error != ERROR_SUCCESS)
	{
		printf(ERROR TIMERTRIB"initialize: Event processor "
			"thread failed to initialize: "
			"ACK'd with error %d.\n", threadAckMsg->error);

		return threadAckMsg->error;
	};

	// Initialize the kernel Timer Stream.
	initializeAllQueuesReq();
	zkcmCore.timerControl.timerQueuesInitializedNotification();

	printf(FATAL TIMERTRIB"initialize: done initializing queues and "
		"sending chipset notification.\n");

	ret = processTrib.__kgetStream()->timerStream.initialize();
	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR TIMERTRIB"initialize: Failed to "
			"initialize timer stream.\n");

		return ret;
	};

	printf(FATAL TIMERTRIB"initialize: done initializing timer "
		"stream.\n");

	return ERROR_SUCCESS;
}

error_t TimerTrib::insertTimerQueueRequestObject(
	TimerStream::sTimerMsg *request
	)
{
	error_t		ret;
	TimerQueue	*suboptimal=NULL;
	
	// Tracing control for this function
	const bool enableTracing = false;

	for (uarch_t i=0; i<TIMERTRIB_TIMERQS_NQUEUES; i++)
	{
		// Skip queues that aren't latched to a device.
		if (!__KBIT_TEST(latchedPeriodMask, i)) { continue; };

		if (enableTracing) {
printf(CC"&1&:%d\n", i);
		}
		suboptimal = timerQueues[i];

		// Temporarily add the queue's period to the placement time.
		request->placementStamp.time.nseconds
			+= timerQueues[i]->getNativePeriod();

		if (enableTracing) {
printf(CC"&2&");
		}

		/* If the object's timeout will be exceeded by placing it into
		 * this queue, skip the queue.
		 **/
		if (request->placementStamp > request->expirationStamp)
		{
			if (enableTracing) {
printf(CC"&3&");
			}
			// Reset the change before continuing.
			request->placementStamp.time.nseconds
				-= timerQueues[i]->getNativePeriod();

			continue;
		};

		if (enableTracing) {
printf(CC"&4&");
		}
		// Same as above: reset the change to placementStamp.
		request->placementStamp.time.nseconds
			-= timerQueues[i]->getNativePeriod();

		// Else, the request can spend at least one tick in this queue.
		timerQueues[i]->lockRequestQueue();
		if (enableTracing) {
printf(CC"&5&");
		}
		ret = timerQueues[i]->insert(request);
		if (enableTracing) {
printf(CC"&6&");
		}
		timerQueues[i]->unlockRequestQueue();
		if (enableTracing) {
printf(CC"&7&");
		}
		return ret;
	};

	if (suboptimal == NULL)
	{
		printf(FATAL TIMERTRIB"No timer queues are latched.\n");
		return ERROR_FATAL;
	};

	/*printf(WARNING TIMERTRIB"Request placed into suboptimal queue %dus."
		"\n", suboptimal->getNativePeriod() / 1000);*/

	if (enableTracing) {
printf(CC"&8&");
	}
	suboptimal->lockRequestQueue();
	if (enableTracing) {
printf(CC"&9&");
	}
	ret = suboptimal->insert(request);
	if (enableTracing) {
printf(CC"&10&");
	}
	suboptimal->unlockRequestQueue();
	if (enableTracing) {
printf(CC"&11&");
	}
	return ret;
}

// Called by Timer Streams to cancel Timer Request objects from Qs.
sarch_t TimerTrib::cancelTimerQueueRequestObject(
	TimerStream::sTimerMsg *request
	)
{
	sarch_t		ret;
	TimerQueue	*targetQueue;

	targetQueue = request->currentQueue;

	targetQueue->lockRequestQueue();
	ret = targetQueue->cancel(request);
	targetQueue->unlockRequestQueue();
	return ret;
}

void TimerTrib::getCurrentTime(sTime *t)
{
	zkcmCore.timerControl.getCurrentTime(t);
}

void TimerTrib::getCurrentDate(sDate *d)
{
	zkcmCore.timerControl.getCurrentDate(d);
}

void TimerTrib::getCurrentDateTime(sTimestamp *stamp)
{
	zkcmCore.timerControl.getCurrentDateTime(stamp);
}

void TimerTrib::dump(void)
{
	printf(NOTICE TIMERTRIB"Dumping.\n");

	printf(NOTICE"\tWatchdog: ");
		if (watchdog.rsrc.isr == NULL) {
			printf((utf8Char *)"No.\n");
		}
		else
		{
			printf((utf8Char *)"Yes: isr addr: %p, "
				"interval: %ds,%dns\n",
				watchdog.rsrc.isr,
				watchdog.rsrc.interval.seconds,
				watchdog.rsrc.interval.nseconds);
		};

	eventProcessor.dump();
}

status_t TimerTrib::registerWatchdogIsr(zkcmIsrFn *isr, sTime interval)
{
	if (isr == NULL) { return ERROR_INVALID_ARG; };
	if ((interval.seconds == 0) && (interval.nseconds == 0)) {
		return ERROR_INVALID_ARG_VAL;
	};

	watchdog.lock.acquire();

	if (watchdog.rsrc.isr != NULL)
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

void TimerTrib::updateWatchdogInterval(sTime interval)
{
	if (interval.seconds == 0 && interval.nseconds == 0) { return; };

	watchdog.lock.acquire();
	memcpy(&watchdog.rsrc.interval, &interval, sizeof(interval));
	watchdog.lock.release();
}

void TimerTrib::unregisterWatchdogIsr(void)
{
	watchdog.lock.acquire();

	if (watchdog.rsrc.isr == NULL)
	{
		watchdog.lock.release();
		return;
	}
	else
	{
		watchdog.rsrc.isr = NULL;

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

