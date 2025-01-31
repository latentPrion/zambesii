
#include <__kstdlib/__kclib/string.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/timerTrib/timerQueue.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


/**	EXPLANATION:
 * Timer Queue is the class used to represent queues of sorted timer request
 * objects from processes (kernel, driver and user). At boot, the kernel will
 * check to see how many timer sources the chipset provides, and how many of
 * them can be used to initialize timer queues. Subsequently, if any new
 * timer devices are detected, the kernel will automatically attempt to see if
 * they can be used to initialize any still-uninitialized timer queues.
 *
 * Any left-over timer sources are exposed to any process which may want to use
 * them via the ZKCM Timer Control mod's TimerFilter api.
 *
 * The number of Timer Queues that the kernel initializes for a chipset is
 * of course, chipset dependent. It is based on the chipset's "safe period bmp",
 * which states what timer periods the chipset can handle safely. Not every
 * chipset can have a timer running at 100Hz and remain stable, for example.
 **/

error_t TimerQueue::latch(ZkcmTimerDevice *dev)
{
	error_t		ret;

	if (dev == NULL) { return ERROR_INVALID_ARG; };

	ret = dev->latch(&processTrib.__kgetStream()->floodplainnStream);
	if (ret != ERROR_SUCCESS)
	{
		printf(WARNING TIMERQUEUE"%dus: latch: to dev \"%s\" failed."
			"\n", nativePeriod / 1000,
			dev->getBaseDevice()->shortName);

		return ret;
	};

	device = dev;
	printf(NOTICE TIMERQUEUE"%dus: latch: latched to device \"%s\".\n",
		getNativePeriod() / 1000, device->getBaseDevice()->shortName);

	return ERROR_SUCCESS;
}

void TimerQueue::unlatch(void)
{
	if (!isLatched()) { return; };

	disable();
	device->unlatch();
	device = NULL;

	printf(NOTICE TIMERQUEUE"%dus: unlatch: unlatched device \"%s\".\n",
		getNativePeriod() / 1000);
}

error_t TimerQueue::enable(void)
{
	error_t		ret;
	sTime		stamp;

	if (!isLatched()) {
		return ERROR_UNINITIALIZED;
	};

	ret = device->setPeriodicMode(sTime(0, nativePeriod));
	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR TIMERQUEUE"%dus: Failed to set periodic mode "
			"on \"%s\".\n",
			getNativePeriod() / 1000,
			getDevice()->getBaseDevice()->shortName);

		return ret;
	};

	ret = device->softEnable();
	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR TIMERQUEUE"%dus: Failed to enable() device.\n",
			getNativePeriod() / 1000);

		return ret;
	};

	return ERROR_SUCCESS;
}

void TimerQueue::disable(void)
{
	/** FIXME: Should first check to see if there are objects left, and wait
	 * for them to expire before physically disabling the timer source.
	 **/
	if (!isLatched()) { return; };
	if (clockRoutineInstalled) { device->softDisable(); }
	else { device->disable(); };
}

error_t TimerQueue::insert(TimerStream::sTimerMsg *request)
{
	error_t		ret;

	request->currentQueue = this;
	ret = requestQueue.addItem(request, request->expirationStamp);
	if (ret != ERROR_SUCCESS)
	{
		request->currentQueue = NULL;
		return ret;
	};

	if (!device->isSoftEnabled()) { return TimerQueue::enable(); };
	return ERROR_SUCCESS;
}

sarch_t TimerQueue::cancel(TimerStream::sTimerMsg *request)
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
	request->currentQueue = NULL;
	if (requestQueue.getNItems() == 0) { disable(); };

	return 1;
}

static ProcessStream *getCreatorProcess(TimerStream::sTimerMsg *request)
{
	// If creator was a power thread, the creator process is the kernel.
	if (Thread::isPowerThread(request->header.sourceId)) {
		return processTrib.__kgetStream();
	};

	// Else, was a normal unique-context thread.
	return processTrib.getStream(request->header.sourceId);
}

static ProcessStream *getTargetProcess(TimerStream::sTimerMsg *request)
{
	if (Thread::isPowerThread(request->header.targetId)) {
		return processTrib.__kgetStream();
	};

	return processTrib.getStream(request->header.targetId);
}

void TimerQueue::tick(sZkcmTimerEvent *event)
{
	TimerStream::sTimerMsg	*request;
	Thread			*targetThread=NULL;
	ProcessStream		*targetProcess, *creatorProcess;
	sarch_t			requestQueueWasEmpty=1;

	/**	EXPLANATION
	 * Get the request at the front of the queue, and if it's expired,
	 * queue an event on the originating process' Timer Stream.
	 *
	 * If the queue is emptied by the sequence, disable the underlying
	 * timer device.
	 **/

	/* The timer queues will only ever have ONE request from any one process
	 * inserted at any time. The rest of a process' timer service requests
	 * are inserted into its Timer Stream's request queue, and when its
	 * currently active request has expired, the kernel will pull a new
	 * request from that process off of its Timer Stream.
	 *
	 * These lock guards prevent any process from inserting a new request
	 * which has an expiry timestamp that expires SOONER than the process'
	 * currently active request for a brief moment to prevent a race.
	 *
	 * If this thread dequeues that currently active request, and while it
	 * is being examined, the process inserts a new item into its queue,
	 * the process' queue will be in a state which is incoherent, and will
	 * cause that new item to be skipped.
	 **/
	lockRequestQueue();
	request = requestQueue.getHead();

	while (request != NULL)
	{

		if (request->expirationStamp > event->irqStamp)
		{
			unlockRequestQueue();
			return;
		};

		// Remove the item.
		request = requestQueue.popFromHead();
		unlockRequestQueue();

		requestQueueWasEmpty = 0;

		// Can occur if the process exited before its object expired.
		targetProcess = getTargetProcess(request);
		if (targetProcess != NULL)
		{
			/* Queue new event on wake-target process' Timer Stream.
			 * "TargetObject" could end up being NULL if the target
			 * thread within the target process exited, or never
			 * existed;
			 *
			 * Can also occur if the target is a CPU and that CPU
			 * was hotplug removed before the request expired.
			 **/
			targetThread = targetProcess->timerStream
				.timerRequestTimeoutNotification(
					request, &event->irqStamp);
		}
		else
		{
			printf(WARNING TIMERQUEUE"%dus: wake target process "
				"%x does not exist.\n",
				getNativePeriod() / 1000,
				request->header.targetId);
		};

		/* For the case where the target process != creator process.
		 * Even if the target process does not exist, we should still
		 * pull a new request from the source process to allow it to
		 * continue placing requests into the queue.
		 **/
		if (PROCID_PROCESS(request->header.sourceId)
			!= PROCID_PROCESS(request->header.targetId))
		{
			creatorProcess = getCreatorProcess(request);
		}
		else { creatorProcess = targetProcess; };

		if (creatorProcess == NULL)
		{
			printf(WARNING TIMERQUEUE"%dus: Inexistent creator "
				"process %x for timer queue request.\n",
				getNativePeriod() / 1000,
				request->header.sourceId);
		}
		else
		{
			// Pull a new request from the creator process.
			creatorProcess->timerStream
				.timerRequestTimeoutNotification();
		};

		if (targetProcess != NULL && targetThread != NULL) {
			taskTrib.unblock(targetThread);
		};

		delete request;

		// See comments above for the reason behind these lock guards.
		lockRequestQueue();
		request = requestQueue.getHead();

		// Disable the queue if it's empty; halt unnecessary IRQ load.
		if (request == NULL) { disable(); };
	};

	unlockRequestQueue();

	if (requestQueueWasEmpty && !atomicAsm::read(&clockRoutineInstalled))
	{
		// If you see this message in the log, don't be too panicked.
		printf(WARNING TIMERQUEUE"%dus: tick called on empty Q without "
			"clock routine.\n",
			getNativePeriod() / 1000);
	};
}

