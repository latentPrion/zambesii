
#include <chipset/zkcm/zkcmCore.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/panic.h>
#include <kernel/common/messageStream.h>
#include <kernel/common/timerTrib/timerTrib.h>
#include <kernel/common/timerTrib/timerStream.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


error_t TimerStream::initialize(void)
{
	return requests.initialize();
}

error_t TimerStream::createOneshotEvent(
	sTimestamp stamp, ubit8 type,
	processId_t targetPid,
	uarch_t flags, void *privateData
	)
{
	sTimerMsg	*request, *tmp;
	error_t		ret;

	if (type > TIMERSTREAM_CREATEONESHOT_TYPE_MAXVAL) {
		return ERROR_INVALID_ARG_VAL;
	};

	request = new sTimerMsg(
		targetPid,
		MSGSTREAM_SUBSYSTEM_TIMER, MSGSTREAM_TIMER_CREATE_ONESHOT_EVENT,
		sizeof(*request), flags, privateData);

	if (request == NULL) { return ERROR_MEMORY_NOMEM; };

	request->type = ONESHOT;

	/*	FIXME:
	 * Security check required here, for when the event is set to wake a
	 * thread in a foreign process.
	 **/

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
	 * reject such a request, but it is probably very normal, and probably
	 * very common for this to happen in applications, so that might be
	 * unreasonable.
	 *
	 * In the current implementation, such a request would just expire as
	 * soon as the next timer event fires.
	 **/

	lockRequestQueue();
	tmp = requests.getHead();

	// If the request queue was empty:
	if (tmp == NULL)
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

error_t TimerStream::pullEvent(ubit32 flags, sTimerMsg **event)
{
	error_t			ret;

	/**	EXPLANATION:
	 * Blocking (or optionally non-blocking if DONT_BLOCK flag is passed)
	 * syscall to allow a thread to pull expired Timer Stream events from
	 * its queue.
	 *
	 * Attempts to pull an event from "events" linked list. If it fails, it
	 * sleeps the process.
	 */
	ret = cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentTaskContext()->messageStream.pullFrom(
			MSGSTREAM_SUBSYSTEM_TIMER,
			(MessageStream::sHeader **)event,
			FLAG_TEST(
				flags, TIMERSTREAM_PULLEVENT_FLAGS_DONT_BLOCK)
					? ZCALLBACK_PULL_FLAGS_DONT_BLOCK : 0);

	if (ret != ERROR_SUCCESS) { return ret; };
	return ERROR_SUCCESS;
}

static inline sarch_t isPerCpuTarget(TimerStream::sTimerMsg *request)
{
	return FLAG_TEST(request->header.flags, MSGSTREAM_FLAGS_CPU_TARGET);
}

void *TimerStream::timerRequestTimeoutNotification(
	sTimerMsg *request, sTimestamp *sTimerMsgtamp
	)
{
	sTimerMsg	*event;
	error_t		ret;
	TaskContext	*taskContext;

	if (isPerCpuTarget(request) && parent->id != __KPROCESSID)
	{
		panic(FATAL TIMERSTREAM"per-cpu request targeting non-kernel "
			"process' timer stream.\n");
	};

	/* Need to get the correct TaskContext object; this could be a context
	 * object that is part of a Thread object, or a context object that
	 * is embedded within a CpuStream object.
	 *
	 * If TIMERSTREAM_CREATE*_FLAGS_CPU_TARGET is set, then the target
	 * context is a per-CPU context. Else, it is a normal thread context.
	 **/
	if (isPerCpuTarget(request))
	{
		CpuStream	*cs;

		cs = cpuTrib.getStream((cpu_t)request->header.targetId);
		if (cs == NULL) { return NULL; };
		taskContext = cs->getTaskContext();
	}
	else
	{
		Thread		*t;

		t = parent->getThread(request->header.targetId);
		if (t == NULL) { return NULL; };
		taskContext = t->getTaskContext();
	};

	event = new sTimerMsg(*request);
	if (event == NULL)
	{
		printf(ERROR TIMERSTREAM"%d: Failed to allocate event for "
			"expired request.\n",
			id);

		// Tbh, it doesn't matter which union member we return.
		return (taskContext->contextType == task::PER_CPU)
			? (void *)taskContext->parent.cpu
			: (void *)taskContext->parent.thread;
	};

	event->actualExpirationStamp = *sTimerMsgtamp;

	// Queue event.
	ret = taskContext->messageStream.enqueue(
		event->header.subsystem, &event->header);

	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR TIMERSTREAM"%d: Failed to add expired event to "
			"thread 0x%x's queue.\n",
			id,
			(taskContext->contextType == task::PER_CPU)
				? taskContext->parent.cpu->cpuId
				: taskContext->parent.thread->getFullId());
	};

	// Tbh, it doesn't matter which union member we return.
	return (taskContext->contextType == task::PER_CPU)
			? (void *)taskContext->parent.cpu
			: (void *)taskContext->parent.thread;
}

void TimerStream::timerRequestTimeoutNotification(void)
{
	sTimerMsg	*nextRequest;

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

	if (nextRequest == NULL)
	{
		printf(FATAL TIMERSTREAM"0x%x: corrupt timer request list:\n"
			"\tpopFromHead() returned NULL when trying to remove "
			"the old item.\n",
			id);

		return;
	};

	/**	NOTES:
	 * We don't delete() the request that was just expired in here because
	 * it's needed still by the timerQ; so we let the timerQ delete it
	 * instead.
	 **/

	nextRequest = requests.getHead();
	if (nextRequest == NULL) { return; };

	if (timerTrib.insertTimerQueueRequestObject(nextRequest)
		!= ERROR_SUCCESS)
	{
		printf(ERROR TIMERSTREAM"%d: Failed to insert the next "
			"timer request object.\n",
			id);
	};
}

