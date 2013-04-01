
#include <chipset/zkcm/zkcmCore.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/timerTrib/timerTrib.h>
#include <kernel/common/timerTrib/timerStream.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
 

error_t timerStreamC::initialize(void)
{
	error_t		ret;

	ret = requests.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	return events.initialize();
}

//void timerStreamC::timerDeviceTimeoutNotification(zkcmTimerEventS *event)
//{
	/**	EXPLANATION:
	 * The process' Timer Stream receives the timer event structure from
	 * the timer device's ISR. It queues the event object as needed, and
	 * then exits.
	 *
	 * It is now up to the process to call "pullTimerEvent()" to process the
	 * timer events that are pending on its stream.
	 *
	 * Returns void because there is really nothing that can be done if
	 * the memory allocation fails. We are currently executing in IRQ
	 * context, so the handler really can't be expected to properly know
	 * what to do with an error return value. At most we can panic, really.
	 *
	 **	TODO:
	 * The API for pulling from the event queue (pullTimerEvent()) has a
	 * "device" argument for filtering the events by device, such that the
	 * caller can ask only for events queued by a certain device. In order
	 * to facilitate this, the kernel needs to (optimally) have a separate
	 * queue instance for each timer device that the process has latched
	 * onto. This way, we can quickly pull events without any issues. An
	 * alternative would be to use prioQueueC, or a derived variant of it.
	 **/
//	events.addItem(event);
//}

error_t timerStreamC::createOneshotEvent(
	timestampS stamp, ubit8 type,
	processId_t wakeTargetThreadId,
	void *privateData, ubit32 /*flags*/
	)
{
	timerObjectS	*request;

	if (type > TIMERSTREAM_CREATEONESHOT_TYPE_MAXVAL) {
		return ERROR_INVALID_ARG_VAL;
	};

	request = new timerObjectS;
	if (request == __KNULL) { return ERROR_MEMORY_NOMEM; };

	request->type = timerObjectS::ONESHOT;
	request->privateData = privateData;
	request->creatorThreadId = cpuTrib.getCurrentCpuStream()
		->taskStream.getCurrentTask()->id;

	/*	FIXME:
	 * Security check required here, for when the event is set to wake a
	 * thread in a foreign process.
	 **/
	request->wakeTargetThreadId = wakeTargetThreadId;

	zkcmCore.timerControl.refreshCachedSystemTime();
	timerTrib.getCurrentTime(&request->placementStamp.time);
	timerTrib.getCurrentDate(&request->placementStamp.date);

	if (type == TIMERSTREAM_CREATEONESHOT_TYPE_RELATIVE)
	{
		request->expirationStamp = request->placementStamp;
		request->expirationStamp.time.seconds += stamp.time.seconds;
		request->expirationStamp.time.nseconds += stamp.time.nseconds;
	}
	else {
		request->expirationStamp = stamp;
	};

	if (requests.getNItems() == 0) {
		return timerTrib.insertTimerQueueRequestObject(request);
	} else {
		return requests.addItem(request, request->expirationStamp);
	};
}

error_t timerStreamC::pullEvent(
	ubit32 flags, eventS *ret
	)
{
	eventS		*event;

	/**	EXPLANATION:
	 * Blocking (or optionally non-blocking if DONT_BLOCK flag is passed)
	 * syscall to allow a process to pull expired timer events from its
	 * Timer Stream.
	 *
	 * Attempts to pull an event from "events" linked list. If it fails, it
	 * sleeps the process.
	 */
	for (;;)
	{
		event = events.popFromHead();
		if (event == __KNULL)
		{
			if (__KFLAG_TEST(
				flags, TIMERSTREAM_PULLEVENT_FLAGS_DONT_BLOCK))
			{
				return ERROR_WOULD_BLOCK;
			};

			taskTrib.block();
		}
		else {
			break;
		};
	};

	*ret = *event;
	delete event;
	return ERROR_SUCCESS;
}

void timerStreamC::timerRequestTimeoutNotification(timerObjectS *request)
{
	eventS		*event;
	error_t		ret;

	event = new eventS;
	if (event == __KNULL)
	{
		__kprintf(ERROR TIMERSTREAM"%d: Failed to allocate event for "
			"expired request.\n",
			id);

		return;
	};

	event->creatorThreadId = request->creatorThreadId;
	event->dueStamp = event->expirationStamp = request->expirationStamp;
	event->privateData = request->privateData;

	// Queue event.
	ret = events.addItem(event);
	if (ret != ERROR_SUCCESS)
	{
		__kprintf(ERROR TIMERSTREAM"%d: Failed to add event for "
			"expired request to event list.\n",
			id);
	};
}

void timerStreamC::timerRequestTimeoutNotification(void)
{
	timerObjectS	*nextRequest;

	nextRequest = requests.popFromHead();
	if (nextRequest == __KNULL) { return; };

	if (timerTrib.insertTimerQueueRequestObject(nextRequest)
		!= ERROR_SUCCESS)
	{
		__kprintf(ERROR TIMERSTREAM"%d: Failed to insert the next "
			"timer request object.\n",
			id);
	};
}

