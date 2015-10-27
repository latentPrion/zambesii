
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/zasyncStream.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


ZAsyncStream::~ZAsyncStream(void)
{
	processId_t		*tmp=NULL;
	uarch_t			nConnections;
	ipc::sDataHeader	*msgTmp;

	connections.lock.acquire();

	if (connections.rsrc.nConnections > 0)
	{
		// Copy the array to tmp, then free the current array.
		nConnections = connections.rsrc.nConnections;
		tmp = new processId_t[nConnections];
		if (tmp != NULL)
		{
			// Copy the array.
			memcpy(
				tmp, connections.rsrc.pids,
				sizeof(*tmp) * nConnections);
		};
	};

	connections.lock.release();

	if (tmp != NULL)
	{
		for (uarch_t i=0; i<nConnections; i++) {
			close(tmp[i]);
		};

		delete[] tmp;
	};

	HeapList<ipc::sDataHeader>::Iterator	it = messages.begin(0);
	for (; it != messages.end(); ++it)
	{
		/*	Fixme:
		 * This should probably be ipc::destroyDataHeader() instead of
		 * a delete call.
		 **/
		msgTmp = *it;
		delete msgTmp;
	};
}

error_t ZAsyncStream::addConnection(processId_t pid)
{
	processId_t		*old;

	/* Resize the array and delete the old one.
	 **/

	connections.lock.acquire();

	old = connections.rsrc.pids;
	connections.rsrc.pids = new processId_t[
		connections.rsrc.nConnections + 1];

	if (connections.rsrc.pids == NULL)
	{
		connections.lock.acquire();
		return ERROR_MEMORY_NOMEM;
	};

	if (connections.rsrc.nConnections > 0)
	{
		memcpy(
			connections.rsrc.pids, old,
			sizeof(*old) * connections.rsrc.nConnections);
	};

	connections.rsrc.pids[connections.rsrc.nConnections] = pid;
	connections.rsrc.nConnections++;

	connections.lock.release();

	delete[] old;
	return ERROR_SUCCESS;
}

void ZAsyncStream::removeConnection(processId_t pid)
{
	connections.lock.acquire();

	for (uarch_t i=0; i<connections.rsrc.nConnections; i++)
	{
		if (PROCID_PROCESS(connections.rsrc.pids[i])
			== PROCID_PROCESS(pid))
		{
			for (uarch_t j=i; j<connections.rsrc.nConnections-1;
				j++)
			{
				connections.rsrc.pids[j] =
					connections.rsrc.pids[j+1];
			};

			connections.rsrc.nConnections--;
			break;
		};
	};

	connections.lock.release();
}

sarch_t ZAsyncStream::findConnection(processId_t pid)
{
	sarch_t		ret=0;

	connections.lock.acquire();

	for (uarch_t i=0; i<connections.rsrc.nConnections; i++)
	{
		if (PROCID_PROCESS(connections.rsrc.pids[i])
			== PROCID_PROCESS(pid))
		{
			ret = 1;
			break;
		};
	};

	connections.lock.release();
	return ret;
}

error_t ZAsyncStream::connect(
	processId_t targetPid, processId_t sourceBindTid, uarch_t flags)
{
	ProcessStream		*targetProcess;
	sZAsyncMsg		*request;

	/**	EXPLANATION:
	 * We want to queue a message on the target. It is up to the target
	 * process to dequeue the message and determine whether or not it will
	 * accept the connection.
	 *
	 *	* If the target process has not decided to listen() for async
	 *	  IPC connection requests, or if its listening thread-ID maps
	 *	  to an inexistent thread, return error.
	 **/
	// If already connected to target process, return error.
	if (findConnection(targetPid))
	{
		printf(WARNING ZASYNC"0x%x: Already connected to pid 0x%x.\n",
			this->id, targetPid);

		return ERROR_DUPLICATE;
	};

	targetProcess = processTrib.getStream(targetPid);
	if (targetProcess == NULL) { return ERROR_INVALID_RESOURCE_NAME; };

	// Ensure both processes are listening for IPC API calls.
	if (targetProcess->zasyncStream.getHandlerTid() == PROCID_INVALID
		|| getHandlerTid() == PROCID_INVALID)
		{ return ERROR_UNINITIALIZED; };

	request = new sZAsyncMsg(
		targetProcess->zasyncStream.getHandlerTid(),
		MSGSTREAM_SUBSYSTEM_ZASYNC, MSGSTREAM_ZASYNC_CONNECT,
		sizeof(*request), flags, NULL);

	if (request == NULL) { return ERROR_MEMORY_NOMEM; };
	request->bindTid = sourceBindTid;

	return MessageStream::enqueueOnThread(
		request->header.targetId, &request->header);
}

error_t ZAsyncStream::respond(
	processId_t initiatorPid,
	connectReplyE reply, processId_t bindTid, uarch_t flags
	)
{
	error_t			ret;
	sZAsyncMsg		*response;
	ProcessStream		*initiatorProcess;

	initiatorProcess = processTrib.getStream(initiatorPid);
	if (initiatorProcess == NULL) { return ERROR_INVALID_RESOURCE_NAME; };

	// Ensure both processes are listening for IPC API calls.
	if (initiatorProcess->zasyncStream.getHandlerTid() == PROCID_INVALID
		|| getHandlerTid() == PROCID_INVALID)
		{ return ERROR_UNINITIALIZED; };

	response = new sZAsyncMsg(
		initiatorPid,
		MSGSTREAM_SUBSYSTEM_ZASYNC, MSGSTREAM_ZASYNC_RESPOND,
		sizeof(*response), flags, NULL);

	if (response == NULL) { return ERROR_MEMORY_NOMEM; };
	response->bindTid = bindTid;
	response->reply = reply;

	if (reply == CONNREPLY_YES)
	{
		ProcessStream		*initiatorProcess;

		// Add both processes to each other's "connections" list.

		initiatorProcess = processTrib.getStream(initiatorPid);
		if (initiatorProcess == NULL)
			{ delete response; return ERROR_NOT_FOUND; };

		ret = initiatorProcess->zasyncStream.addConnection(
			response->header.sourceId);

		if (ret != ERROR_SUCCESS) { delete response; return ret; };

		ret = addConnection(initiatorPid);
		if (ret != ERROR_SUCCESS) { delete response; return ret; };
	};

	return MessageStream::enqueueOnThread(
		response->header.targetId, &response->header);
}

error_t ZAsyncStream::send(
	processId_t bindTid, void *data, uarch_t nBytes, ipc::methodE method,
	uarch_t flags, void *privateData
	)
{
	sZAsyncMsg		*message;
	ProcessStream		*targetProcess;
	ipc::sDataHeader	*dataHeader;
	error_t			ret;

	if (data == NULL) { return ERROR_INVALID_ARG; };

	targetProcess = processTrib.getStream(bindTid);
	if (targetProcess == NULL) { return ERROR_INVALID_RESOURCE_ID; };

	/* First check to see if the target is connectionless. If the target
	 * is not listening connectionless, then the sender must be connected.
	 **/
	if (!targetProcess->zasyncStream.connectionlessListenEnabled()
		&& !findConnection(bindTid))
	{ return ERROR_UNINITIALIZED; };

	dataHeader = new (ipc::createDataHeader(data, nBytes, method))
		ipc::sDataHeader;

	if (dataHeader == NULL) { return ERROR_MEMORY_NOMEM; };

	message = new sZAsyncMsg(
		bindTid,
		MSGSTREAM_SUBSYSTEM_ZASYNC, MSGSTREAM_ZASYNC_SEND,
		sizeof(*message), flags, privateData);

	if (message == NULL) { delete dataHeader; return ERROR_MEMORY_NOMEM; };
	message->dataHandle = dataHeader;
	message->dataNBytes = dataHeader->nBytes;
	message->method = dataHeader->method;
	dataHeader->requestMessage = message;

	ret = targetProcess->zasyncStream.messages.insert(dataHeader);
	if (ret != ERROR_SUCCESS)
		{ delete dataHeader; delete message; return ret; };

	return MessageStream::enqueueOnThread(
		message->header.targetId, &message->header);
}

error_t ZAsyncStream::receive(void *dataHandle, void *buffer, uarch_t)
{
	ipc::sDataHeader		*dataHeader;
	error_t				ret;

	if (dataHandle == NULL || buffer == NULL) { return ERROR_INVALID_ARG; };

	dataHeader = (ipc::sDataHeader *)dataHandle;
	if (!messages.checkForItem(dataHeader))
		{ return ERROR_INVALID_RESOURCE_HANDLE; };

	// Dispatch the data.
	ret = ipc::dispatchDataHeader(dataHeader, buffer);
	if (ret != ERROR_SUCCESS) { return ret; };
	return ERROR_SUCCESS;
}

void ZAsyncStream::acknowledge(void *dataHandle, void *buffer, void *privateData)
{
	ipc::sDataHeader		*dataHeader;
	ipc::methodE			method;
	processId_t			sourceTid;
	sZAsyncMsg			*response;

	if (dataHandle == NULL) { return; };

	dataHeader = (ipc::sDataHeader *)dataHandle;

	if (!messages.checkForItem(dataHeader)) { return; };

	method = dataHeader->method;
	sourceTid = dataHeader->foreignTid;
	ipc::destroyDataHeader(dataHeader, buffer);

	// For BUFFER, there is nothing else to do here.
	if (method == ipc::METHOD_BUFFER) { return; };

	// Send a notification to the sender.
	response = new sZAsyncMsg(
		sourceTid,
		MSGSTREAM_SUBSYSTEM_ZASYNC, MSGSTREAM_ZASYNC_ACKNOWLEDGE,
		sizeof(*response), 0, privateData);

	if (response == NULL)
	{
		printf(ERROR ZASYNC"0x%x: response message alloc failed; "
			"async sender 0x%x may be halted indefinitely.\n",
			this->id, sourceTid);

		return;
	};

	MessageStream::enqueueOnThread(
		response->header.targetId, &response->header);
}

void ZAsyncStream::close(processId_t)
{
}

