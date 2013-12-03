
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/zasyncStream.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


zasyncStreamC::~zasyncStreamC(void)
{
	processId_t		*tmp=NULL;
	uarch_t			nConnections;
	void			*handle;
	messageHeaderS		*msgTmp;

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

	handle = NULL;
	while ((msgTmp = messages.getNextItem(&handle)) != NULL)
		{ delete[] msgTmp; };
}

error_t zasyncStreamC::addConnection(processId_t pid)
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

void zasyncStreamC::removeConnection(processId_t pid)
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

sarch_t zasyncStreamC::findConnection(processId_t pid)
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

error_t zasyncStreamC::connect(
	processId_t targetPid, processId_t sourceBindTid, uarch_t flags)
{
	processStreamC		*targetProcess;
	taskC			*targetTask;
	zasyncMsgS		*request;

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

	targetTask = targetProcess->getTask(
		targetProcess->zasyncStream.getHandlerTid());

	// Don't allow IPC connections to CPUs.
	if (targetTask->getType() != task::UNIQUE)
		{ return ERROR_INVALID_OPERATION; };

	request = new zasyncMsgS(
		targetProcess->zasyncStream.getHandlerTid(),
		MSGSTREAM_SUBSYSTEM_ZASYNC, MSGSTREAM_ZASYNC_CONNECT,
		sizeof(*request), flags, NULL);

	if (request == NULL) { return ERROR_MEMORY_NOMEM; };

	request->bindTid = sourceBindTid;
	return messageStreamC::enqueueOnThread(
		request->header.targetId, &request->header);
}

error_t zasyncStreamC::respond(
	processId_t initiatorPid,
	connectReplyE reply, processId_t bindTid, uarch_t flags
	)
{
	error_t			ret;
	zasyncMsgS		*response;
	processStreamC		*initiatorProcess;

	initiatorProcess = processTrib.getStream(initiatorPid);
	if (initiatorProcess == NULL) { return ERROR_INVALID_RESOURCE_NAME; };

	// Ensure both processes are listening for IPC API calls.
	if (initiatorProcess->zasyncStream.getHandlerTid() == PROCID_INVALID
		|| getHandlerTid() == PROCID_INVALID)
		{ return ERROR_UNINITIALIZED; };

	response = new zasyncMsgS(
		initiatorPid,
		MSGSTREAM_SUBSYSTEM_ZASYNC, MSGSTREAM_ZASYNC_RESPOND,
		sizeof(*response), flags, NULL);

	if (response == NULL) { return ERROR_MEMORY_NOMEM; };

	response->bindTid = bindTid;
	response->reply = reply;

	if (reply == CONNREPLY_YES)
	{
		processStreamC		*initiatorProcess;

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

	return messageStreamC::enqueueOnThread(
		response->header.targetId, &response->header);
}

error_t zasyncStreamC::send(
	processId_t bindTid, void *data, uarch_t nBytes, uarch_t flags
	)
{
	zasyncMsgS		*message;
	processStreamC		*targetProcess;
	messageHeaderS		*dataHeader;
	ubit8			*dataBuff;
	error_t			ret;

	if (data == NULL) { return ERROR_INVALID_ARG; };
	if (!findConnection(bindTid)) { return ERROR_UNINITIALIZED; };

	targetProcess = processTrib.getStream(bindTid);
	if (targetProcess == NULL) { return ERROR_INVALID_RESOURCE_NAME; };

	dataBuff = new ubit8[sizeof(messageHeaderS) + nBytes];
	if (dataBuff == NULL) { return ERROR_MEMORY_NOMEM; };
	dataHeader = (messageHeaderS *)dataBuff;
	dataBuff = &dataBuff[sizeof(messageHeaderS)];

	message = new zasyncMsgS(
		bindTid,
		MSGSTREAM_SUBSYSTEM_ZASYNC, MSGSTREAM_ZASYNC_SEND,
		sizeof(*message), flags, NULL);

	if (message == NULL) { delete[] dataBuff; return ERROR_MEMORY_NOMEM; };

	ret = targetProcess->zasyncStream.messages.insert(dataHeader);
	if (ret != ERROR_SUCCESS)
		{ delete[] dataBuff; delete message; return ret; };

	dataHeader->nBytes = nBytes;
	memcpy(dataBuff, data, nBytes);
	message->dataNBytes = nBytes;
	message->dataHandle = dataBuff;

	return messageStreamC::enqueueOnThread(
		message->header.targetId, &message->header);
}

error_t zasyncStreamC::receive(void *dataHandle, void *buffer, uarch_t)
{
	messageHeaderS		*dataHeader;

	if (dataHandle == NULL || buffer == NULL) { return ERROR_INVALID_ARG; };

	dataHeader = (messageHeaderS *)dataHandle;
	if (!messages.checkForItem(dataHeader))
		{ return ERROR_INVALID_RESOURCE_HANDLE; };

	// Copy the data and remove the message from the list.
	memcpy(buffer, &dataHeader[1], dataHeader->nBytes);
	messages.remove(dataHeader);
	return ERROR_SUCCESS;
}

void zasyncStreamC::close(processId_t)
{
}

