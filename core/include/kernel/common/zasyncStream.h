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

class processStreamC;

class zasyncStreamC
:
public streamC
{
public:
	enum connectResponseE { CONNRES_YES, CONNRES_NO };

	zasyncStreamC(processId_t parentPid, processStreamC *parent)
	:
	streamC(parentPid),
	handlerTid(PROCID_INVALID),
	parent(parent)
	{}

	error_t initialize(void) { return ERROR_SUCCESS; };
	~zasyncStreamC(void);

public:
	struct zasyncMsgS
	{
		zasyncMsgS(
			processId_t targetPid, ubit16 subsystem, ubit16 function,
			uarch_t size, uarch_t flags, void *privateData)
		:
		header(targetPid, subsystem, function, size, flags, privateData)
		{}

		messageStreamC::headerS		header;
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
		processId_t			boundTid;
		void				*dataHandle;
	};

public:
	void listen(processId_t handlerTid)
		{ this->handlerTid = handlerTid; }

	// Called by the initiator to establish a connection with the target.
	#define MSGSTREAM_ZASYNC_CONNECT		(0)
	error_t connect(processId_t targetPid, uarch_t flags);
	// Called by target to allow/deny a connect request from the initiator.
	#define MSGSTREAM_ZASYNC_RESPOND		(1)
	error_t respond(
		processId_t initiatorPid,
		connectResponseE response, processId_t pairTid, uarch_t flags);

	#define MSGSTREAM_ZASYNC_SEND		(2)
	error_t send(
		processId_t pairTid, void *data, uarch_t nBytes, uarch_t flags);

	void receive(void *msgHandle, void *buffer, uarch_t flags);
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
	struct stateS
	{
		stateS(void)
		:
		pids(NULL), nConnections(0)
		{}

		processId_t	*pids;
		uarch_t		nConnections;
	};

	sharedResourceGroupC<waitLockC, stateS>	connections;
	ptrListC<ubit8>		messages;
	processStreamC		*parent;
};

#endif

