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

#define TIMERSTREAM_CREATEONESHOT_FLAGS_CPU_TARGET	(1<<0)
#define TIMERSTREAM_CREATEONESHOT_FLAGS_CPU_SOURCE	(1<<1)

#define TIMERSTREAM_CREATEONESHOT_TYPE_ABSOLUTE		(0x0)
#define TIMERSTREAM_CREATEONESHOT_TYPE_RELATIVE		(0x1)
#define TIMERSTREAM_CREATEONESHOT_TYPE_MAXVAL		(0x1)

#define TIMERSTREAM_CREATEPERIODIC_FLAGS_CPU_TARGET	(1<<0)
#define TIMERSTREAM_CREATEPERIODIC_FLAGS_CPU_SOURCE	(1<<1)

class timerQueueC;
class processStreamC;
class threadC;

class timerStreamC
:
public streamC
{
friend class timerTribC;
friend class timerQueueC;
public:
	timerStreamC(processId_t id, processStreamC *parent)
	:
	streamC(id), parent(parent)
	{}

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
		ubit32			flags;
	};

	// Used to represent expired timer request events.
	struct eventS
	{
		enum eventTypeE { PERIODIC=1, ONESHOT } type;
		processId_t	creatorThreadId;
		timestampS	dueStamp, expirationStamp;
		void		*privateData;
		ubit32		flags;
	};

	// sarch_t nanosleep(ubit32 delay);
	// sarch_t microsleep(ubit32 delay);
	// sarch_t millisleep(ubit32 delay);
	// sarch_t sleep(timeS delay);

	error_t createAbsoluteOneshotEvent(
		timestampS stamp, void *wakeTarget,
		void *privateData, ubit32 flags)
	{
		return createOneshotEvent(
			stamp, TIMERSTREAM_CREATEONESHOT_TYPE_ABSOLUTE,
			wakeTarget, privateData, flags);
	}

	error_t createRelativeOneshotEvent(
		timestampS stamp, void *wakeTarget,
		void *privateData, ubit32 flags)
	{
		return createOneshotEvent(
			stamp, TIMERSTREAM_CREATEONESHOT_TYPE_RELATIVE,
			wakeTarget, privateData, flags);
	}

	/* wakeTarget:
	 *	Target thread (threadC) or cpu (cpuStreamC) which must be
	 *	awakened by this timer, and on whose callbackStream the
	 *	notification message will be queued.
	 *
	 *	If TIMERSTREAM_CREATEONESHOT_FLAGS_CPU_TARGET is set, then:
	 *		* If wakeTarget is NULL, the target CPU to wake is
	 *		  assumed to be the current CPU. The behaviour is the
	 *		  same as passing the current CPU's cpuStream pointer.
	 *		* If wakeTarget is set, it is assumed to be a cpuStreamC
	 *		  pointer for the CPU on which to queue the callback and
	 *		  wake on event expiry.
	 *	If TIMERSTREAM_CREATEONESHOT_FLAGS_CPU_TARGET is not set, then:
	 *		* If wakeTarget is NULL, the target thread is assumed to
	 *		  be the calling thread. The behaviour is the same as
	 *		  the calling thread passing itself as an argument.
	 *		* If wakeTarget is set, it is assumed to be a threadC
	 *		  pointer for the thread to queue the callback on, and
	 *		  wake when the event expires.
	 *
	 * This argument works the same way for createPeriodicEvent.
	 **/
	error_t createOneshotEvent(
		timestampS stamp, ubit8 type, void *wakeTarget,
		void *privateData, ubit32 flags);

	error_t createPeriodicEvent(
		timeS interval,
		void *wakeTarget,
		void *privateData, ubit32 flags);

	// Pulls an event from the expired events list.
	error_t pullEvent(ubit32 flags, eventS *ret);

private:
	/* Used by the Timer Tributary's dequeueing thread to lock the stream
	 * against insertion of new requests while a request from this process
	 * is being expired. Prevents a series of race conditions from occuring.
	 **/
	void lockRequestQueue(void) { requestQueueLock.acquire(); };
	void unlockRequestQueue(void) { requestQueueLock.release(); };

	/* Queues a timer request expiry event on this stream and returns a
	 * pointer to the object that it unblocked. That is, it returns a
	 * pointer to the threadC or cpuStreamC object that it unblocked
	 * after queueing the new callback.
	 **/
	void *timerRequestTimeoutNotification(requestS *request);
	// Causes this stream to insert its next request into the timer queues.
	void timerRequestTimeoutNotification(void);

private:
	sortedPointerDoubleListC<requestS, timestampS>	requests;
	/* Used to prevent race conditions while requests from this process are
	 * being expired.
	 **/
	waitLockC					requestQueueLock;
	processStreamC					*parent;
};

#endif

