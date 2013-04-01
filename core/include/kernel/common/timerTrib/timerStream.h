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

class timerStreamC
:
public streamC
{
friend class timerQueueC;
public:
	timerStreamC(void) {}
	error_t initialize(void);

public:
	struct eventS
	{
		processId_t	creatorThreadId;
		timestampS	dueStamp, expirationStamp;
		void		*privateData;
	};

	// sarch_t sleep(timeS delayLength);			
	// error_t setPeriodicTimer(timeS period, ubit32 flags, void **handle);
	// error_t setOneshotTimer(timeS delayLength, ubit32 flags, void **handle);


	/**	EXPLANATION:
	 * Pulls an object from the process's timer expiry event queue. Will
	 * dormant the calling thread if there are no items in the queue, unless
	 * TIMERSTREAM_PULLTIMEREVENT_DONT_BLOCK is specified. If the "device"
	 * argument is supplied, it is expected to contain the handle for a
	 * timer device that was latched onto by the calling process. In this
	 * case, the kernel will only return timer timeout event objects for
	 * that device.
	 *
	 * Returns ERROR_SUCCESS when an object is pulled, else it dormants the
	 * thread (or if DONT_BLOCK was specified it returns ERROR_WOULD_BLOCK).
	 **/
	// error_t pullDeviceEvent(
	//	void *device, ubit32 flags, zkcmTimerEventS *ret);

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
	// void timerDeviceTimeoutNotification(zkcmTimerEventS *event);
	// Queues a timer request expiry event on this stream.
	void timerRequestTimeoutNotification(timerObjectS *request);
	// Causes this stream to insert its next request into the timer queues.
	void timerRequestTimeoutNotification(void);

private:
	pointerDoubleListC<eventS>		events;
	sortedPointerDoubleListC<timerObjectS, timestampS>	requests;
};

#endif

