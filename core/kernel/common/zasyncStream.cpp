
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
	ubit8			*msgTmp;

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

error_t zasyncStreamC::connect(processId_t targetPid, uarch_t flags)
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
	if (targetProcess == NULL) { return ERROR_NOT_FOUND; };

	if (targetProcess->zasyncStream.getHandlerTid() == PROCID_INVALID)
		{ return ERROR_UNINITIALIZED; };

	targetTask = targetProcess->getTask(
		targetProcess->zasyncStream.getHandlerTid());

	// Don't allow IPC connections to CPUs.
	if (targetTask->getType() != task::UNIQUE)
		{ return ERROR_INVALID_OPERATION; };

	request = new zasyncMsgS(
		targetPid, MSGSTREAM_SUBSYSTEM_ZASYNC, MSGSTREAM_ZASYNC_CONNECT,
		sizeof(*request), flags, NULL);

	if (request == NULL) { return ERROR_MEMORY_NOMEM; };

	return messageStreamC::enqueueOnThread(
		request->header.targetId, &request->header);
}

error_t zasyncStreamC::respond(
	processId_t initiatorPid,
	connectReplyE reply, processId_t pairTid, uarch_t flags
	)
{
	error_t			ret;
	zasyncMsgS		*response;

	response = new zasyncMsgS(
		initiatorPid,
		MSGSTREAM_SUBSYSTEM_ZASYNC, MSGSTREAM_ZASYNC_RESPOND,
		sizeof(*response), flags, NULL);

	if (response == NULL) { return ERROR_MEMORY_NOMEM; };

	response->boundTid = pairTid;
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

void zasyncStreamC::close(processId_t)
{
}

