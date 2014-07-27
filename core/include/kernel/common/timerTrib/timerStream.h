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

class TimerQueue;
class ProcessStream;
class Thread;

class TimerStream
:
public Stream
{
friend class TimerTrib;
friend class TimerQueue;
public:
	TimerStream(processId_t id, ProcessStream *parent)
	:
	Stream(id), parent(parent)
	{}

	error_t initialize(void);

public:
	enum requestTypeE { PERIODIC=1, ONESHOT };
	// Used to represent timer service requests.
	struct sTimerMsg
	{
		sTimerMsg(
			processId_t targetPid, ubit16 subsystem, ubit16 function,
			uarch_t size, uarch_t flags, void *privateData)
		:
		header(targetPid, subsystem, function, size, flags, privateData)
		{}

		MessageStream::sHeader	header;
		requestTypeE		type;
		sTimestamp		placementStamp, expirationStamp,
					actualExpirationStamp;
		TimerQueue		*currentQueue;
	};

	// sarch_t nanosleep(ubit32 delay);
	// sarch_t microsleep(ubit32 delay);
	// sarch_t millisleep(ubit32 delay);
	// sarch_t sleep(sTime delay);

	error_t createAbsoluteOneshotEvent(
		sTimestamp stamp, processId_t targetPid,
		ubit32 flags, void *privateData)
	{
		return createOneshotEvent(
			stamp, TIMERSTREAM_CREATEONESHOT_TYPE_ABSOLUTE,
			targetPid, flags, privateData);
	}

	error_t createRelativeOneshotEvent(
		sTimestamp stamp, processId_t targetPid,
		ubit32 flags, void *privateData)
	{
		return createOneshotEvent(
			stamp, TIMERSTREAM_CREATEONESHOT_TYPE_RELATIVE,
			targetPid, flags, privateData);
	}

	/* wakeTarget:
	 *	Target thread (Thread) or cpu (CpuStream) which must be
	 *	awakened by this timer, and on whose messageStream the
	 *	notification message will be queued.
	 *
	 *	If MSGSTREAM_FLAGS_CPU_TARGET is set, then:
	 *		* If wakeTarget is NULL, the target CPU to wake is
	 *		  assumed to be the current CPU. The behaviour is the
	 *		  same as passing the current CPU's CpuStream pointer.
	 *		* If wakeTarget is set, it is assumed to be a cpuStream
	 *		  pointer for the CPU on which to queue the callback and
	 *		  wake on event expiry.
	 *	If MSGSTREAM_FLAGS_CPU_TARGET is not set, then:
	 *		* If wakeTarget is NULL, the target thread is assumed to
	 *		  be the calling thread. The behaviour is the same as
	 *		  the calling thread passing itself as an argument.
	 *		* If wakeTarget is set, it is assumed to be a Thread
	 *		  pointer for the thread to queue the callback on, and
	 *		  wake when the event expires.
	 *
	 * This argument works the same way for createPeriodicEvent.
	 **/
	#define MSGSTREAM_TIMER_CREATE_ONESHOT_EVENT	(0)
	error_t createOneshotEvent(
		sTimestamp stamp, ubit8 type, processId_t targetPid,
		ubit32 flags, void *privateData);

	#define MSGSTREAM_TIMER_CREATE_PERIODIC_EVENT	(1)
	error_t createPeriodicEvent(
		sTime interval, processId_t targetPid,
		ubit32 flags, void *privateData);

	// Pulls an event from the expired events list.
	error_t pullEvent(ubit32 flags, sTimerMsg *ret);

private:
	/* Used by the Timer Tributary's dequeueing thread to lock the stream
	 * against insertion of new requests while a request from this process
	 * is being expired. Prevents a series of race conditions from occuring.
	 **/
	void lockRequestQueue(void) { requestQueueLock.acquire(); };
	void unlockRequestQueue(void) { requestQueueLock.release(); };

	/* Queues a timer request expiry event on this stream and returns a
	 * pointer to the object that it unblocked. That is, it returns a
	 * pointer to the Thread or cpuStream object that it unblocked
	 * after queueing the new callback.
	 **/
	void *timerRequestTimeoutNotification(
		sTimerMsg *request, sTimestamp *eventStamp);

	// Causes this stream to insert its next request into the timer queues.
	void timerRequestTimeoutNotification(void);

private:
	SortedPtrDblList<sTimerMsg, sTimestamp>	requests;
	/* Used to prevent race conditions while requests from this process are
	 * being expired.
	 **/
	WaitLock					requestQueueLock;
	ProcessStream					*parent;
};

#endif

