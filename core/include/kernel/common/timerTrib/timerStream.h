#ifndef _TIMER_STREAM_H
	#define _TIMER_STREAM_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/sortedPtrDoubleList.h>
	#include <__kclasses/ptrDoubleList.h>
	#include <kernel/common/stream.h>
	#include <kernel/common/timerTrib/timeTypes.h>

#define TIMERSTREAM		"TimerStream "

#define TIMERSTREAM_PULLDEVICEEVENT_FLAGS_DONT_BLOCK	(1<<0)

#define TIMERSTREAM_PULLEVENT_FLAGS_DONT_BLOCK		(1<<0)

#define TIMERSTREAM_CREATEONESHOT_TYPE_ABSOLUTE		(0x0)
#define TIMERSTREAM_CREATEONESHOT_TYPE_RELATIVE		(0x1)
#define TIMERSTREAM_CREATEONESHOT_TYPE_MAXVAL		(0x1)

class timerQueueC;

class timerStreamC
:
public streamC
{
friend class timerQueueC;
public:
	timerStreamC(void) {}
	error_t initialize(void);

public:
	// Used to represent timer service requests.
	struct requestS
	{
		enum requestTypeE { PERIODIC=1, ONESHOT } type;
		// ThreadID to wake up when this object expires.
		processId_t		creatorThreadId, wakeTargetThreadId;
		timestampS		expirationStamp, placementStamp;
		timerQueueC		*currentQueue;
		void			*privateData;
	};

	// Used to represent expired timer request events.
	struct eventS
	{
		enum eventTypeE { PERIODIC=1, ONESHOT } type;
		processId_t	creatorThreadId;
		timestampS	dueStamp, expirationStamp;
		void		*privateData;
	};

	// sarch_t nanosleep(ubit32 delay);
	// sarch_t microsleep(ubit32 delay);
	// sarch_t millisleep(ubit32 delay);
	// sarch_t sleep(timeS delay);

	error_t createAbsoluteOneshotEvent(
		timestampS stamp, processId_t wakeTargetThreadId,
		void *privateData, ubit32 flags)
	{
		return createOneshotEvent(
			stamp, TIMERSTREAM_CREATEONESHOT_TYPE_ABSOLUTE,
			wakeTargetThreadId, privateData, flags);
	}

	error_t createRelativeOneshotEvent(
		timestampS stamp, processId_t wakeTargetThreadId,
		void *privateData, ubit32 flags)
	{
		return createOneshotEvent(
			stamp, TIMERSTREAM_CREATEONESHOT_TYPE_RELATIVE,
			wakeTargetThreadId, privateData, flags);
	}

	error_t createOneshotEvent(
		timestampS stamp, ubit8 type, processId_t wakeTargetThreadId,
		void *privateData, ubit32 flags);

	error_t createPeriodicEvent(
		timeS interval,
		processId_t wakeTargetThreadId,
		void *privateData, ubit32 flags);

	// Pulls an event from the expired events list.
	error_t pullEvent(ubit32 flags, eventS *ret);

private:
	// Queues a timer request expiry event on this stream.
	void timerRequestTimeoutNotification(timerStreamC::requestS *request);
	// Causes this stream to insert its next request into the timer queues.
	void timerRequestTimeoutNotification(void);

private:
	pointerDoubleListC<eventS>			events;
	sortedPointerDoubleListC<requestS, timestampS>	requests;
};

#endif

