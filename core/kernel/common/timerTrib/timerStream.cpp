
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
		->taskStream.getCurrentTask()->id;

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

	tmp = requests.getHead();

	/**	FIXME:
	 * It is possible for the pull of objects from a process to be halted
	 * due to a race condition; specifically, where the insertion of a
	 * new request is called for, and there is already a request for this
	 * process being serviced by the timer queues; however, before the new
	 * request is added to the request list, the old request is expired,
	 * and the kernel asks the process for a new request, but since the
	 * insertion hasn't happened yet, nothing is pulled.
	 *
	 * Thus the process is silently blocked from any Timer Stream syscalls.
	 * This can be remedied using an idle thread which periodically checks
	 * to ensure that all processes refresh their queue requests.
	 **/

	// If the request queue was empty:
	if (tmp == __KNULL)
	{
		ret = requests.addItem(request, request->expirationStamp);
		if (ret != ERROR_SUCCESS) { return ret; };

		return timerTrib.insertTimerQueueRequestObject(request);
	};

	// If the new request expires before the one currently being serviced:
	if (request->expirationStamp < tmp->expirationStamp)
	{
		// FIXME: Review here for a possible race condition.
		timerTrib.cancelTimerQueueRequestObject(tmp);
		ret = requests.addItem(request, request->expirationStamp);
		if (ret != ERROR_SUCCESS) { return ret; };

		return timerTrib.insertTimerQueueRequestObject(request);
	}
	else {
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

void timerStreamC::timerRequestTimeoutNotification(requestS *request)
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
	requestS	*nextRequest;

	// Pop the request that just timed out.
	nextRequest = requests.popFromHead();
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

