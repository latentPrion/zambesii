#ifndef _TIMER_STREAM_H
	#define _TIMER_STREAM_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/sortedPtrDoubleList.h>
	#include <__kclasses/ptrDoubleList.h>
	#include <kernel/common/stream.h>
	#include <kernel/common/zmessage.h>
	#include <kernel/common/timerTrib/timeTypes.h>

#define TIMERSTREAM		"TimerStream "

#define TIMERSTREAM_PULLDEVICEEVENT_FLAGS_DONT_BLOCK	(1<<0)

#define TIMERSTREAM_PULLEVENT_FLAGS_DONT_BLOCK		(1<<0)

#define TIMERSTREAM_CREATEONESHOT_TYPE_ABSOLUTE		(0x0)
#define TIMERSTREAM_CREATEONESHOT_TYPE_RELATIVE		(0x1)
#define TIMERSTREAM_CREATEONESHOT_TYPE_MAXVAL		(0x1)

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
	enum requestTypeE { PERIODIC=1, ONESHOT };
	// Used to represent timer service requests.
	struct requestS
	{
		zmessage::headerS	header;
		requestTypeE		type;
		timestampS		expirationStamp, placementStamp;
		timerQueueC		*currentQueue;
	};

	// Used to represent expired timer request events.
	struct eventS
	{
		zmessage::headerS	header;
		requestTypeE		type;
		timestampS		dueStamp, actualStamp;
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
	 *	awakened by this timer, and on whose messageStream the
	 *	notification message will be queued.
	 *
	 *	If ZMESSAGE_FLAGS_CPU_TARGET is set, then:
	 *		* If wakeTarget is NULL, the target CPU to wake is
	 *		  assumed to be the current CPU. The behaviour is the
	 *		  same as passing the current CPU's cpuStream pointer.
	 *		* If wakeTarget is set, it is assumed to be a cpuStreamC
	 *		  pointer for the CPU on which to queue the callback and
	 *		  wake on event expiry.
	 *	If ZMESSAGE_FLAGS_CPU_TARGET is not set, then:
	 *		* If wakeTarget is NULL, the target thread is assumed to
	 *		  be the calling thread. The behaviour is the same as
	 *		  the calling thread passing itself as an argument.
	 *		* If wakeTarget is set, it is assumed to be a threadC
	 *		  pointer for the thread to queue the callback on, and
	 *		  wake when the event expires.
	 *
	 * This argument works the same way for createPeriodicEvent.
	 **/
	#define ZMESSAGE_TIMER_CREATE_ONESHOT_EVENT	(0)
	error_t createOneshotEvent(
		timestampS stamp, ubit8 type, void *wakeTarget,
		void *privateData, ubit32 flags);

	#define ZMESSAGE_TIMER_CREATE_PERIODIC_EVENT	(1)
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
	void *timerRequestTimeoutNotification(
		requestS *request, timestampS *eventStamp);

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

