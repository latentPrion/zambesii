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

/**	Flags for messageStreamC::pull().
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
 *	ID of the CPU or thread that made the asynch call-in that prompted this
 *	asynch round-trip.
 * targetId (only for zrequest::headerS).
 *	ID of the target CPU or thread that the callback/response for this
 *	asynch round-trip must be sent to.
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

/**	Values for zrequest::headerS::subsystem & zcallback::headerS::subsystem.
 * Subsystem IDs are basically implicitly queue IDs (to the kernel; externally
 * no assumptions should be made about the mapping of subsystem IDs to queues
 * in the kernel).
 **/
// Keep this up to date.
#define MSGSTREAM_SUBSYSTEM_MAXVAL		(0x7)
// Actual subsystem values.
#define MSGSTREAM_SUBSYSTEM_USER0		(0x0)
#define MSGSTREAM_SUBSYSTEM_USER1		(0x1)
#define MSGSTREAM_SUBSYSTEM_USER2		(0x2)
#define MSGSTREAM_SUBSYSTEM_USER3		(0x3)
#define MSGSTREAM_SUBSYSTEM_TIMER		(0x4)
#define MSGSTREAM_SUBSYSTEM_PROCESS		(0x5)
#define MSGSTREAM_SUBSYSTEM_FLOODPLAINN		(0x6)
#define MSGSTREAM_SUBSYSTEM_ZASYNC		(0x7)

#define MSGSTREAM_USERQ(num)			(MSGSTREAM_SUBSYSTEM_USER0 + num)

class taskContextC;
class taskC;

namespace ipc
{
	enum methodE {
		METHOD_BUFFER, METHOD_MAP_AND_COPY, METHOD_MAP_AND_READ };

	struct dataHeaderS
	{
		methodE		method;
		void		*foreignVaddr;
		uarch_t		nBytes;
		/* This next attribute is only needed for MAP_AND_COPY and
		 * MAP_AND_READ.
		 **/
		processId_t	foreignTid;
	};

	dataHeaderS *createDataHeader(
		void *data, uarch_t nBytes, methodE method);

	// Does *NOT* delete the header before returning.
	error_t dispatchDataHeader(dataHeaderS *header, void *buffer);
	void destroyDataHeader(dataHeaderS *header, void *buffer);
}

class messageStreamC
{
public:
	// Common header contained within all messages.
	struct headerS
	{
		headerS(
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

	struct iteratorS
	{
		iteratorS(void)
		:
		header(0, 0, 0, 0, 0, NULL)
		{}

		headerS		header;
		ubit8		_padding_[256];
	};

	typedef pointerDoubleListC<messageStreamC::headerS>	messageQueueC;

public:
	messageStreamC(taskContextC *parent)
	:
	parent(parent)
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

	~messageStreamC(void) {};

public:
	error_t pull(messageStreamC::iteratorS *callback, ubit32 flags=0);
	error_t pullFrom(
		ubit16 subsystemQueue, messageStreamC::iteratorS *callback,
		ubit32 flags=0);

	error_t	enqueue(ubit16 queueId, messageStreamC::headerS *callback);

	// Utility functions exported for other subsystems to use.
	static error_t enqueueOnThread(
		processId_t targetCallbackStream,
		messageStreamC::headerS *header);

	static processId_t determineSourceThreadId(
		taskC *caller, ubit16 *flags);

	static processId_t determineTargetThreadId(
		processId_t targetId, processId_t sourceId,
		uarch_t callerFlags, ubit16 *messageFlags);

private:
	messageQueueC *getSubsystemQueue(ubit16 subsystemId)
	{
		if (subsystemId > MSGSTREAM_SUBSYSTEM_MAXVAL)
			{ return NULL; }

		return &queues[subsystemId];
	}

private:
	messageQueueC	queues[MSGSTREAM_SUBSYSTEM_MAXVAL + 1];

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

typedef void (voidF)(void);
typedef void (syscallbackFuncF)(messageStreamC::iteratorS *msg, void (*func)());
typedef void (syscallbackDataF)(messageStreamC::iteratorS *msg, void *data);

class syscallbackC
{
public:
	syscallbackC(syscallbackDataF *syscallback)
	:
	func(NULL), data(NULL)
		{ this->syscallback.dataOverloadedForm = syscallback; }

	syscallbackC(syscallbackFuncF *syscallback, void (*func)())
	:
	func(func), data(NULL)
		{ this->syscallback.funcOverloadedForm = syscallback; }

	syscallbackC(syscallbackDataF *syscallback, void *data)
	:
	func(NULL), data(data)
		{ this->syscallback.dataOverloadedForm = syscallback; }

	void operator ()(messageStreamC::iteratorS *msg)
	{
		/* If there is no function to call, just exit. This comparison
		 * may be unportable because it doesn't also compare the
		 * funcOverloadedForm member of the union.
		 */
		if (syscallback.dataOverloadedForm == NULL) { return; };

		// The order of the conditions is important here.
		if ((data == NULL && func == NULL) || data != NULL)
			{ syscallback.dataOverloadedForm(msg, NULL); return; };

		syscallback.funcOverloadedForm(msg, func);
	}

private:
	union
	{
		syscallbackFuncF	*funcOverloadedForm;
		syscallbackDataF	*dataOverloadedForm;
	} syscallback;
	void		(*func)();
	void		*data;
};

class slamCacheC;
extern slamCacheC	*asyncContextCache;

#endif

