#ifndef _CALLBACK_STREAM_H
	#define _CALLBACK_STREAM_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/bitmap.h>
	#include <__kclasses/ptrDoubleList.h>
	#include <kernel/common/processId.h>
	#include <kernel/common/stream.h>

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

/**	Flags for callbackStreamC::headerS.
 **/
#define ZCALLBACK_FLAGS_CPU_SOURCE		(1<<0)
#define ZCALLBACK_FLAGS_CPU_TARGET		(1<<1)

#define ZREQUEST_FLAGS_CPU_SOURCE		(1<<0)
#define ZREQUEST_FLAGS_CPU_TARGET		(1<<1)

/**	Values for callbackStreamC::headerS.
 * Subsystems ID are basically implicitly queue IDs (to the kernel; externally
 * no assumptions should be made about the mapping of subsystem IDs to queues
 * in the kernel).
 **/
#define ZCALLBACK_SUBSYSTEM_MAXVAL		(0x1)
#define ZCALLBACK_SUBSYSTEM_TIMER		(0x0)
#define ZCALLBACK_SUBSYSTEM_PROCESS		(0x1)

/**	Flags for callbackStreamC::pull().
 **/
#define ZCALLBACK_PULL_FLAGS_DONT_BLOCK		(1<<0)

class taskContextC;

class callbackStreamC
{
public:
	callbackStreamC(taskContextC *parent)
	:
	parent(parent)
	{}

	error_t initialize(void)
	{
		error_t		ret;

		for (ubit16 i=0; i<ZCALLBACK_SUBSYSTEM_MAXVAL + 1; i++)
		{
			ret = queues[i].initialize(
				PTRDBLLIST_INITIALIZE_FLAGS_USE_OBJECT_CACHE);

			if (ret != ERROR_SUCCESS) { return ret; };
		};

		return pendingSubsystems.initialize(
			ZCALLBACK_SUBSYSTEM_MAXVAL + 1);
	};

	~callbackStreamC(void) {};

public:
	// The base message type included in all callback messages.
	struct headerS
	{
		/**	EXPLANATION:
		 * This structure must remain a plain old data type.
		 *
		 * sourceId:
		 *	ID of the CPU or thread that made the asynch syscall
		 *	that triggered this callback. Can also be a CPU ID if
		 *	ZCALLBACK_FLAGS_CPU_SOURCE is set.
		 * privateData:
		 *	Data that is private to the source thread which was
		 *	preserved across the syscall.
		 * flags: Self-explanatory.
		 * err:
		 *	Most API callbacks return an error status; this member
		 *	reduces the need for custom messages by enabling
		 *	callbacks to use it instead of specialized messages.
		 * subsystem + function:
		 *	Used by the source thread to determine which function
		 *	call the callback is for. Most subsystems will not have
		 *	more than 0xFFFF functions and those that do can overlap
		 *	their functions into a new subsystem ID, such that they
		 *	use more than one subsystem ID (while remaining a single
		 *	subsystem).
		 *
		 *	Also, since every new subsystem ID implies a separate,
		 *	fresh queue, subsystems which wish to isolate specific
		 *	groups of functions into their own queues can overlap
		 *	those functions into a new subsystem ID if so desired.
		 **/
		processId_t	sourceId;
		void		*privateData;
		ubit32		flags;
		error_t		err;
		ubit16		subsystem, function;
	};

	struct genericCallbackS
	{
		headerS		header;
	};

public:
	pointerDoubleListC<headerS> *getSubsystem(ubit8 subsystemId)
	{
		if (subsystemId > ZCALLBACK_SUBSYSTEM_MAXVAL)
			{ return __KNULL; }

		return &queues[subsystemId];
	}

	error_t pull(headerS **callback, ubit32 flags=0);
	error_t	enqueue(headerS *callback);

	// Utility function exported for other subsystems to use.
	static error_t enqueueCallback(
		processId_t targetCallbackStream, headerS *header);

private:
	pointerDoubleListC<headerS>	queues[ZCALLBACK_SUBSYSTEM_MAXVAL + 1];

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

