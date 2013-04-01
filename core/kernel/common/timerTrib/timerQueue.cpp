
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

error_t timerQueueC::latch(zkcmTimerDeviceC *dev)
{
	error_t		ret;

	if (dev == __KNULL) { return ERROR_INVALID_ARG; };

	ret = dev->latch(&processTrib.__kprocess.floodplainnStream);
	if (ret != ERROR_SUCCESS)
	{
		__kprintf(WARNING TIMERQUEUE"%dus: latch: to dev \"%s\" failed."
			"\n", nativePeriod / 1000,
			dev->getBaseDevice()->shortName);

		return ret;
	};

	device = dev;
	__kprintf(NOTICE TIMERQUEUE"%dus: latch: latched to device \"%s\".\n",
		getNativePeriod() / 1000, device->getBaseDevice()->shortName);

	return ERROR_SUCCESS;
}

void timerQueueC::unlatch(void)
{
	if (!isLatched()) { return; };

	disable();
	device->unlatch();
	device = __KNULL;

	__kprintf(NOTICE TIMERQUEUE"%dus: unlatch: unlatched device \"%s\".\n",
		getNativePeriod() / 1000);
}

error_t timerQueueC::enable(void)
{
	error_t		ret;
	timeS		stamp;

	if (!isLatched()) {
		return ERROR_UNINITIALIZED;
	};

	ret = device->setPeriodicMode(timeS(0, nativePeriod));
	if (ret != ERROR_SUCCESS)
	{
		__kprintf(ERROR TIMERQUEUE"%dus: Failed to set periodic mode "
			"on \"%s\".\n",
			getNativePeriod() / 1000,
			getDevice()->getBaseDevice()->shortName);

		return ret;
	};

	ret = device->enable();
	if (ret != ERROR_SUCCESS)
	{
		__kprintf(ERROR TIMERQUEUE"%dus: Failed to enable() device.\n",
			getNativePeriod() / 1000);

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

error_t timerQueueC::insert(timerObjectS *request)
{
	error_t		ret;

	ret = requestQueue.addItem(request, request->expirationStamp);
	if (ret != ERROR_SUCCESS) { return ret; };

	if (!device->isEnabled()) { return enable(); };
	return ERROR_SUCCESS;
}

sarch_t timerQueueC::cancel(timerObjectS *request)
{
	/**	FIXME:
	 * This function is intended to return 1 if the item being canceled was
	 * truly canceled (it had not yet expired and been processed), or 0 if
	 * it had already been processed and the "cancel" operation ended up
	 * being a NOP.
	 *
	 * Sadly, the list class doesn't return a value from removeItem().
	 **/
	requestQueue.removeItem(request);
	if (requestQueue.getNItems() == 0) { disable(); };
	return 1;
}

void timerQueueC::tick(zkcmTimerEventS *event)
{
	timerObjectS	*request;
	taskC		*targetTask;
	processStreamC	*targetProcess, *creatorProcess;

	/**	EXPLANATION
	 * Get the request at the front of the queue, and if it's expired,
	 * queue an event on the originating process' Timer Stream.
	 *
	 * If the queue is emptied by the sequence, disable the underlying
	 * timer device.
	 **/
	request = requestQueue.getHead();
	if (request == __KNULL)
	{
		__kprintf(WARNING TIMERQUEUE"%dus: tick called on empty Q.\n",
			getNativePeriod() / 1000);

		return;
	};

	if (request->expirationStamp > event->irqStamp) { return; };

	// Remove the item.
	request = requestQueue.popFromHead();

	// Can occur if the process exited before its object expired.
	targetProcess = processTrib.getStream(request->wakeTargetThreadId);
	if (targetProcess == __KNULL)
	{
		__kprintf(WARNING TIMERQUEUE"%dus: wake target process 0x%x "
			"does not exist.\n",
			getNativePeriod() / 1000, request->wakeTargetThreadId);

		return;
	};

	// Can occur if the thread exited before its object expired.
	targetTask = targetProcess->getTask(request->wakeTargetThreadId);
	if (targetTask == __KNULL)
	{
		__kprintf(WARNING TIMERQUEUE"%dus: Inexistent thread 0x%x.\n",
			getNativePeriod() / 1000, request->wakeTargetThreadId);

		return;
	};

	// Queue an event on the wake-target process' Timer Stream.
	targetProcess->timerStream.timerRequestTimeoutNotification(request);
	delete request;

	// For the case where the target process != creator process.
	if (PROCID_PROCESS(request->creatorThreadId)
		!= PROCID_PROCESS(request->wakeTargetThreadId))
	{
		creatorProcess = processTrib.getStream(
			request->creatorThreadId);

		if (creatorProcess == __KNULL)
		{
			__kprintf(WARNING TIMERQUEUE"%dus: Inexistent creating "
				"process 0x%x for timer queue request.\n",
				getNativePeriod() / 1000,
				request->creatorThreadId);
		}
		else
		{
			// Makes the process insert its next request into the Qs
			creatorProcess->timerStream
				.timerRequestTimeoutNotification();
		};
	}
	else
	{
		// Makes the process insert its next request into the Qs.
		targetProcess->timerStream.timerRequestTimeoutNotification();
	};

	// Disable the queue if it's empty; halt unnecessary IRQ load.
	if (requestQueue.getHead() == __KNULL) { disable(); };
	// Unblock the target thread.
	taskTrib.unblock(targetTask);
}

