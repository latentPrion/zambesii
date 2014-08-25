#ifndef _MESSAGE_STREAM_H
	#define _MESSAGE_STREAM_H

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
 *
 *	CAVEAT:
 * Because of the way we must handle garbage collection of messages in non-
 * handshaken APIs, we have to require that under no circumstances should
 * processes be allowed to "peek" into the MessageStream of a thread in another
 * process.
 *
 * This is because if a kernel-domain process calls a receive() operation on a
 * the MessageStream of a user-domain process' thread, it will cause a
 * double-free. This is because the user-domain wrapper for MessageStream::pull
 * automatically calls ::delete on the message.
 **/

/**	Flags for MessageStream::pull().
 **/
#define ZCALLBACK_PULL_FLAGS_DONT_BLOCK		(1<<0)

#define MSGSTREAM				"MsgStream "

#define MSGSTREAM_FLAGS_CPU_SOURCE		(1<<0)
#define MSGSTREAM_FLAGS_CPU_TARGET		(1<<1)

/**	EXPLANATION:
 * All of these structures must remain POD (plain-old-data) types. They are used
 * as part of the userspace ABI. In general, meaning of the header members is
 * as follows:
 *
 * sourceId:
 *	ID of the CPU or thread that made the async call-in that prompted this
 *	async round-trip.
 * targetId (only for zrequest::sHeader).
 *	ID of the target CPU or thread that the callback/response for this
 *	async round-trip must be sent to.
 * flags: (common subset):
 *	Z*_FLAGS_CPU_SOURCE: The source object was a CPU and not a thread.
 *	Z*_FLAGS_CPU_TARGET: The target object is a CPU and not a thread.
 * privateData:
 *	Opaque storage (sizeof(void *)) that is uninterpeted by the kernel
 *	subsystem that processes the request; this will be copied from the
 *	request object and into the callback object.
 * subsystem + function:
 *	Used by the source thread to determine which function call the callback
 *	is for. Most subsystems will not have more than 0xFFFF functions and
 *	those that do can overlap their functions into a new subsystem ID, such
 *	that they use more than one subsystem ID (while remaining a single
 *	subsystem).
 *
 *	Also, since every new subsystem ID implies a separate, fresh queue,
 *	subsystems which wish to isolate specific groups of functions into their
 *	own queues can overlap those functions into a new subsystem ID if so
 *	desired.
 **/

/**	Values for zrequest::sHeader::subsystem & zcallback::sHeader::subsystem.
 * Subsystem IDs are basically implicitly queue IDs (to the kernel; externally
 * no assumptions should be made about the mapping of subsystem IDs to queues
 * in the kernel).
 **/
// Keep these two up to date.
#define MSGSTREAM_SUBSYSTEM_MAXVAL		(0x8)
#define MSGSTREAM_USERQ_MAXVAL			(0x3)
// Actual subsystem values.
#define MSGSTREAM_SUBSYSTEM_USER0		(0x0)
#define MSGSTREAM_SUBSYSTEM_USER1		(0x1)
#define MSGSTREAM_SUBSYSTEM_USER2		(0x2)
#define MSGSTREAM_SUBSYSTEM_USER3		(0x3)
#define MSGSTREAM_SUBSYSTEM_TIMER		(0x4)
#define MSGSTREAM_SUBSYSTEM_PROCESS		(0x5)
#define MSGSTREAM_SUBSYSTEM_FLOODPLAINN		(0x6)
#define MSGSTREAM_SUBSYSTEM_ZASYNC		(0x7)
#define MSGSTREAM_SUBSYSTEM_ZUDI		(0x8)

#define MSGSTREAM_USERQ(num)			(MSGSTREAM_SUBSYSTEM_USER0 + num)

class Thread;

namespace ipc
{
	enum methodE {
		METHOD_BUFFER, METHOD_MAP_AND_COPY, METHOD_MAP_AND_READ };

	struct sDataHeader
	{
		methodE		method;
		void		*foreignVaddr;
		/* We use this to ensure garbage collection of the request
		 * message. C++ doesn't allow us to forward declare member
		 * structs, so this unfortunately, has to be untyped.
		 **/
		void		*requestMessage;
		uarch_t		nBytes;
		/* This next attribute is only needed for MAP_AND_COPY and
		 * MAP_AND_READ.
		 **/
		processId_t	foreignTid;
	};

	sDataHeader *createDataHeader(
		void *data, uarch_t nBytes, methodE method);

	// Does *NOT* delete the header before returning.
	error_t dispatchDataHeader(sDataHeader *header, void *buffer);
	void destroyDataHeader(sDataHeader *header, void *buffer);
}

class MessageStream
: public Stream<Thread>
{
public:
	// Common header contained within all messages.
	struct sHeader
	{
		sHeader(
			processId_t targetPid,
			ubit16 subsystem, ubit16 function,
	 		uarch_t size, uarch_t flags, void *privateData);

		processId_t	sourceId, targetId;
		void		*privateData;
		error_t		error;
		ubit16		subsystem, flags;
		ubit32		function;
		uarch_t		size;
	};

	typedef PtrDblList<MessageStream::sHeader>	MessageQueue;

public:
	MessageStream(Thread *parent)
	: Stream<Thread>(parent, 0)
	{}

	error_t initialize(void)
	{
		error_t		ret;

		for (ubit16 i=0; i<MSGSTREAM_SUBSYSTEM_MAXVAL + 1; i++)
		{
			ret = queues[i].initialize(
				PTRDBLLIST_INITIALIZE_FLAGS_USE_OBJECT_CACHE);

			if (ret != ERROR_SUCCESS) { return ret; };
		};

		return pendingSubsystems.initialize(
			MSGSTREAM_SUBSYSTEM_MAXVAL + 1);
	};

	~MessageStream(void) {};

public:
	error_t pull(MessageStream::sHeader **message, ubit32 flags=0);
	error_t pullFrom(
		ubit16 subsystemQueue, MessageStream::sHeader **message,
		ubit32 flags=0);

	error_t	enqueue(ubit16 queueId, MessageStream::sHeader *message);
	error_t postMessage(
		processId_t tid, ubit16 userQueueId,
		ubit16 messageNo, void *data,
		error_t errorVal=ERROR_SUCCESS);

	// Utility functions exported for other subsystems to use.
	static error_t enqueueOnThread(
		processId_t targetMessageStream,
		MessageStream::sHeader *header);

	static processId_t determineSourceThreadId(
		Thread *caller, ubit16 *flags);

	static processId_t determineTargetThreadId(
		processId_t targetId, processId_t sourceId,
		uarch_t callerFlags, ubit16 *messageFlags);

private:
	MessageQueue *getSubsystemQueue(ubit16 subsystemId)
	{
		if (subsystemId > MSGSTREAM_SUBSYSTEM_MAXVAL)
			{ return NULL; }

		return &queues[subsystemId];
	}

private:
	MessageQueue	queues[MSGSTREAM_SUBSYSTEM_MAXVAL + 1];

	/* Bitmap of all subsystem queues which have messages in them. The lock
	 * on this bitmap is also used as the serializing lock that minimizes
	 * the chances of lost wakeup races occuring.
	 *
	 * Callbacks have to acquire this lock before inserting a new message
	 * in any queue, and they hold it until they have inserted the
	 * new message.
	 **/
	Bitmap			pendingSubsystems;
};

#define DONT_SEND_RESPONSE		((void *)NULL)

/**	EXPLANATION:
 * Basically, you give this class a pointer to a block of memory which holds
 * an asynchronous response message. The message will automatically be sent when
 * an instance of the class destructs, UNLESS the internal pointer is
 * NULL. If the internal pointer is NULL, the message will not be sent.
 *
 * To set the internal message pointer, use:
 *	object.setMessage(POINTER_TO_BLOCK_OF_MEMORY).
 * To set the error value that is set in the message's header, use:
 *	object.setMessage(YOUR_CHOSEN_ERROR_VALUE).
 *
 * You can also use operator() as a shorthand:
 *	object(POINTER_TO_BLOCK_OF_MEMORY) or,
 *	object(YOUR_CHOSEN_ERROR_VALUE).
 *
 * If you already have a message pointer stored internally from a previous
 * setMessage(), you can clear it by calling:
 *	setMessage(DONT_SEND_RESPONSE), which is the same as
 *	setMessage((void *)NULL);
 **/
class AsyncResponse
{
public:
	AsyncResponse(void) : msgHeader(NULL) {}
	~AsyncResponse(void);

	void operator() (error_t e) { setMessage(e); }
	void operator() (void *msg) { setMessage(msg); }

	void setMessage(error_t e)
		{ err = e; }

	void setMessage(void *msg)
		{ msgHeader = msg; }

private:
	void		*msgHeader;
	error_t		err;
};

#endif

