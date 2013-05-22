
#include <chipset/zkcm/zkcmCore.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/timerTrib/timerTrib.h>
#include <kernel/common/timerTrib/timerStream.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
 

error_t timerStreamC::initialize(void)
{
	return requests.initialize();
}

error_t timerStreamC::createOneshotEvent(
	timestampS stamp, ubit8 type,
	processId_t wakeTargetThreadId,
	void *privateData, ubit32 /*flags*/
	)
{
	requestS	*request, *tmp;
	error_t		ret;

	if (type > TIMERSTREAM_CREATEONESHOT_TYPE_MAXVAL) {
		return ERROR_INVALID_ARG_VAL;
	};

	request = new requestS;
	if (request == __KNULL) { return ERROR_MEMORY_NOMEM; };

	request->type = requestS::ONESHOT;
	request->privateData = privateData;
	request->creatorThreadId = cpuTrib.getCurrentCpuStream()
		->taskStream.getCurrentTask()->getFullId();

	/*	FIXME:
	 * Security check required here, for when the event is set to wake a
	 * thread in a foreign process.
	 **/
	request->wakeTargetThreadId = wakeTargetThreadId;

	timerTrib.getCurrentDateTime(&request->placementStamp);

	if (type == TIMERSTREAM_CREATEONESHOT_TYPE_RELATIVE)
	{
		request->expirationStamp = request->placementStamp;
		request->expirationStamp.time.seconds += stamp.time.seconds;
		request->expirationStamp.time.nseconds += stamp.time.nseconds;
	}
	else {
		request->expirationStamp = stamp;
	};

	/**	NOTE:
	 * It might have been expedient to do an error check here for the case
	 * where a request asks for an expiry time that expires in the past and
	 * reject such a request, but it is probably very possible, and probably
	 * very common for this to happen in applications, so that might be
	 * unreasonable.
	 **/

	lockRequestQueue();
	tmp = requests.getHead();

	// If the request queue was empty:
	if (tmp == __KNULL)
	{
		ret = requests.addItem(request, request->expirationStamp);
		unlockRequestQueue();
		if (ret != ERROR_SUCCESS) { return ret; };

		return timerTrib.insertTimerQueueRequestObject(request);
	};

	// If the new request expires before the one currently being serviced:
	if (request->expirationStamp < tmp->expirationStamp)
	{
		timerTrib.cancelTimerQueueRequestObject(tmp);
		ret = requests.addItem(request, request->expirationStamp);
		unlockRequestQueue();
		if (ret != ERROR_SUCCESS) { return ret; };

		return timerTrib.insertTimerQueueRequestObject(request);
	}
	else
	{
		ret = requests.addItem(request, request->expirationStamp);
		unlockRequestQueue();
		return ret;
	};
}

error_t timerStreamC::pullEvent(
	ubit32 flags, eventS *ret
	)
{
	eventS				*event;
	pointerDoubleListC<eventS>	*queue;

	/**	EXPLANATION:
	 * Blocking (or optionally non-blocking if DONT_BLOCK flag is passed)
	 * syscall to allow a thread to pull expired Timer Stream events from
	 * its queue.
	 *
	 * Attempts to pull an event from "events" linked list. If it fails, it
	 * sleeps the process.
	 */
	queue = &cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask()
		->timerStreamEvents;

	for (;;)
	{
		event = queue->popFromHead();
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

void timerStreamC::timerRequestTimeoutNotification(
	requestS *request, taskC *targetThread
	)
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

	event->type = (eventS::eventTypeE)request->type;
	event->creatorThreadId = request->creatorThreadId;
	event->dueStamp = event->expirationStamp = request->expirationStamp;
	event->privateData = request->privateData;

	// Queue event.
	ret = targetThread->timerStreamEvents.addItem(event);
	if (ret != ERROR_SUCCESS)
	{
		__kprintf(ERROR TIMERSTREAM"%d: Failed to add expired event to "
			"thread 0x%x's queue.\n",
			id, targetThread->getFullId());
	};
}

void timerStreamC::timerRequestTimeoutNotification(void)
{
	requestS	*nextRequest;

	/* Lock the request queue against insertions from the process to prevent
	 * a race condition. If a process is trying to insert a request into its
	 * queue, and there is only one item in the queue, then this next
	 * stretch of code will pop that last item out of the queue.
	 *
	 * If this pop occurs at a specific point in the insertion (just after
	 * the insertion code checks to see if the queue was empty), the pop
	 * operation will invalidate the result of that check, and cause the
	 * insertion code to only insert the request on the Timer Stream
	 * internal queue. This will in turn cause the kernel never to pull
	 * requests from this process again, effectively silencing this process
	 * from the kernel's timer services.
	 **/
	lockRequestQueue();
	// Pop the request that just timed out.
	nextRequest = requests.popFromHead();
	unlockRequestQueue();

	if (nextRequest == __KNULL)
	{
		__kprintf(FATAL TIMERSTREAM"0x%x: corrupt timer request list:\n"
			"\tpopFromHead() returned NULL when trying to remove "
			"the old item.\n",
			id);

		return;
	};

	delete nextRequest;

	nextRequest = requests.getHead();
	if (nextRequest == __KNULL) { return; };

	if (timerTrib.insertTimerQueueRequestObject(nextRequest)
		!= ERROR_SUCCESS)
	{
		__kprintf(ERROR TIMERSTREAM"%d: Failed to insert the next "
			"timer request object.\n",
			id);
	};
}

