#ifndef _TIMER_STREAM_H
	#define _TIMER_STREAM_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/sortedPtrDoubleList.h>
	#include <kernel/common/stream.h>
	#include <kernel/common/timerTrib/timeTypes.h>

// #define TIMERSTREAM_SETONESHOT_FLAGS_ASYNCH		(1<<0)

#define TIMERSTREAM_PULL_TIMEREVENT_DONT_BLOCK		(1<<0)

#define TIMERSTREAM_PULL_TIMERQUEUEEVENT_DONT_BLOCK	(1<<0)

class timerStreamC
:
public streamC
{
public:
	timerStreamC(void) {}
	error_t initialize(void);

public:
	// sarch_t sleep(timeS delayLength);			
	// error_t setPeriodicTimer(timeS period, ubit32 flags, void **handle);
	// error_t setOneshotTimer(timeS delayLength, ubit32 flags, void **handle);

	// void timerDeviceTimeoutNotification(zkcmTimerEventS *event);

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
	// error_t pullTimerEvent(
	//	void *device, ubit32 flags, zkcmTimerEventS *ret);

	// error_t pullTimerQueueObject(ubit32 flags, timerQueueObjectS *ret);

private:
	// pointerDoubleListC<zkcmTimerEventS>	events;
};

#endif

