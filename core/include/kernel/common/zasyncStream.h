#ifndef _Z_ASYNC_STREAM_H
	#define _Z_ASYNC_STREAM_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/ptrList.h>
	#include <kernel/common/stream.h>
	#include <kernel/common/processId.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>
	#include <kernel/common/messageStream.h>

#define ZASYNC			"ZAsync "

class ProcessStream;

class ZAsyncStream
:
public Stream
{
public:
	enum connectReplyE { CONNREPLY_YES, CONNREPLY_NO, CONNREPLY_CLOSED };

	ZAsyncStream(processId_t parentPid, ProcessStream *parent)
	:
	Stream(parentPid),
	handlerTid(PROCID_INVALID),
	parent(parent)
	{}

	error_t initialize(void) { return messages.initialize(); };
	~ZAsyncStream(void);

public:
	struct zasyncMsgS
	{
		zasyncMsgS(
			processId_t targetPid, ubit16 subsystem, ubit16 function,
			uarch_t size, uarch_t flags, void *privateData)
		:
		header(targetPid, subsystem, function, size, flags, privateData)
		{}

		MessageStream::sHeader		header;
		/* When a process calls respond(), it must pass in the ID of one
		 * of its threads which it intends for the initiator to send its
		 * messages to. The initiator is expected to save this thread's
		 * ID and use it when sending messages.
		 *
		 * Should the ID of that thread change (e.g, thread was
		 * terminated, and the process started a new thread instead),
		 * a notification can be sent out by simply sending a second
		 * respond() message to interested parties. The onus then falls
		 * on those parties to save the new thread ID and send messages
		 * to that thread. Any kind of recovery should be negotiated
		 * by the involved processes using some custom arrangement.
		 **/
		processId_t			bindTid;
		ipc::methodE			method;
		connectReplyE			reply;
		void				*dataHandle;
		uarch_t				dataNBytes;
	};

public:
	sarch_t connectionlessListenEnabled(void)
	{
		sarch_t		ret;

		connections.lock.acquire();
		ret = connections.rsrc.connectionlessListen;
		connections.lock.release();
		return ret;
	}

	void listen(processId_t handlerTid, sarch_t connectionless=0)
	{
		this->handlerTid = handlerTid;
		connections.lock.acquire();
		connections.rsrc.connectionlessListen = connectionless;
		connections.lock.release();
	}

	// Called by the initiator to establish a connection with the target.
	#define MSGSTREAM_ZASYNC_CONNECT		(0)
	error_t connect(
		processId_t targetPid, processId_t sourceBindTid,
		uarch_t flags);

	// Called by target to allow/deny a connect request from the initiator.
	#define MSGSTREAM_ZASYNC_RESPOND		(1)
	error_t respond(
		processId_t initiatorPid,
		connectReplyE reply, processId_t bindTid, uarch_t flags);

	#define MSGSTREAM_ZASYNC_SEND			(2)
	error_t send(
		processId_t bindTid, void *data, uarch_t nBytes,
		ipc::methodE method, uarch_t flags,
		void *privateData);

	error_t receive(void *dataHandle, void *buffer, uarch_t flags);
	#define MSGSTREAM_ZASYNC_ACKNOWLEDGE		(3)
	void acknowledge(void *dataHandle, void *buffer, void *privateData);
	void close(processId_t pid);

	processId_t getHandlerTid(void) { return handlerTid; };

	void dump(void);

private:
	error_t addConnection(processId_t pid);
	void removeConnection(processId_t pid);
	sarch_t findConnection(processId_t pid);

private:
	processId_t		handlerTid;
	/* Array of PIDs of processes to which this process has been
	 * successfully connected, whether it was the initiator or not.
	 **/
	struct sState
	{
		sState(void)
		:
		pids(NULL), nConnections(0), connectionlessListen(0)
		{}

		processId_t	*pids;
		uarch_t		nConnections;
		sarch_t		connectionlessListen;
	};

	SharedResourceGroup<WaitLock, sState>	connections;
	PtrList<ipc::dataHeaderS>	messages;
	ProcessStream			*parent;
};

#endif

