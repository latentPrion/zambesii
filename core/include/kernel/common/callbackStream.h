#ifndef _CALLBACK_STREAM_H
	#define _CALLBACK_STREAM_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/bitmap.h>
	#include <__kclasses/ptrDoubleList.h>
	#include <kernel/common/processId.h>
	#include <kernel/common/stream.h>
	#include <kernel/common/zmessage.h>

/**	EXPLANATION:
 * Per-thread kernel API callback queue manager that enqueues callbacks for the
 * thread to pick up. It maintains a bitmap of "pending callbacks" that can
 * be used to quickly check which queues are occupied. The thread can then pull
 * from the queues to receive callbacks from the kernel.
 *
 * This class also makes it very easy for us to allow CPUs to be the targets
 * of callbacks, and for CPUs to be able to make calls to kernel asynchronous
 * APIs.
 **/

/**	Flags for callbackStreamC::pull().
 **/
#define ZCALLBACK_PULL_FLAGS_DONT_BLOCK		(1<<0)

class taskContextC;

class callbackStreamC
{
public:
	typedef pointerDoubleListC<zcallback::headerS>	callbackQueueC;

	callbackStreamC(taskContextC *parent)
	:
	parent(parent)
	{}

	error_t initialize(void)
	{
		error_t		ret;

		for (ubit16 i=0; i<ZMESSAGE_SUBSYSTEM_MAXVAL + 1; i++)
		{
			ret = queues[i].initialize(
				PTRDBLLIST_INITIALIZE_FLAGS_USE_OBJECT_CACHE);

			if (ret != ERROR_SUCCESS) { return ret; };
		};

		return pendingSubsystems.initialize(
			ZMESSAGE_SUBSYSTEM_MAXVAL + 1);
	};

	~callbackStreamC(void) {};

public:
	callbackQueueC *getSubsystemQueue(ubit8 subsystemId)
	{
		if (subsystemId > ZMESSAGE_SUBSYSTEM_MAXVAL)
			{ return __KNULL; }

		return &queues[subsystemId];
	}

	error_t pull(zcallback::headerS **callback, ubit32 flags=0);
	error_t	enqueue(zcallback::headerS *callback);

	// Utility function exported for other subsystems to use.
	static error_t enqueueCallback(
		processId_t targetCallbackStream,
		zcallback::headerS *header);

private:
	callbackQueueC	queues[ZMESSAGE_SUBSYSTEM_MAXVAL + 1];

	/* Bitmap of all subsystem queues which have messages in them. The lock
	 * on this bitmap is also used as the serializing lock that minimizes
	 * the chances of lost wakeup races occuring.
	 *
	 * Callbacks have to acquire this lock before inserting a new message
	 * in any queue, and they hold it until they have inserted the
	 * new message.
	 **/
	bitmapC				pendingSubsystems;
	taskContextC			*parent;
};

#endif

