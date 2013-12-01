#ifndef _Z_ASYNC_STREAM_H
	#define _Z_ASYNC_STREAM_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/ptrList.h>
	#include <kernel/common/stream.h>
	#include <kernel/common/processId.h>

class zasyncStreamC
:
public streamC
{
public:
	enum connectResponseE { CONNRES_YES, CONNRES_NO };

	zasyncStreamC();
	error_t initialize(void);

public:
	void listen(processId_t handlerTid);

	// Called by the initiator to establish a connection with the target.
	error_t connect(processId_t targetPid, uarch_t flags);
	// Called by target to allow/deny a connect request from the initiator.
	error_t respond(
		processId_t initiatorPid,
		connectResponseE response, processId_t pairTid, uarch_t flags);

	error_t send(
		processId_t pairTid, void *data, uarch_t nBytes, uarch_t flags);

	void receive(void *msgHandle, void *buffer, uarch_t flags);
	void close(processId_t pid);

	void dump(void);

private:
	processId_t		handlerTid;
	/* Array of PIDs of processes to which this process has been
	 * successfully connected, whether it was the initiator or not.
	 **/
	processId_t		*connectedProcesses;
	ptrListC<void>		queuedMessages;
};

#endif

