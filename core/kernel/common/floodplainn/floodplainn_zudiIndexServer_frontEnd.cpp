
#include <kernel/common/thread.h>
#include <kernel/common/process.h>
#include <kernel/common/zudiIndexServer.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/floodplainn/floodplainn.h>


error_t zudiIndexServer::detectDriverReq(
	utf8Char *devicePath, indexE index,
	void *privateData, ubit32 flags
	)
{
	zudiIndexMsgS			*request;
	taskC				*currTask;

	if (strnlen8(devicePath, ZUDIIDX_SERVER_MSG_DEVICEPATH_MAXLEN)
		>= ZUDIIDX_SERVER_MSG_DEVICEPATH_MAXLEN)
		{ return ERROR_LIMIT_OVERFLOWED; };

	currTask = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask();

	// Allocate and queue the new request.
	request = new zudiIndexMsgS(
		ZUDIIDX_SERVER_DETECTDRIVER_REQ, devicePath, index);

	if (request == NULL) { return ERROR_MEMORY_NOMEM; };

	return currTask->parent->zasyncStream.send(
		floodplainn.zudiIndexServerTid, request, sizeof(*request),
		ipc::METHOD_BUFFER, flags, privateData);
}

error_t zudiIndexServer::loadDriverReq(utf8Char *devicePath, void *privateData)
{
	zudiIndexServer::zudiIndexMsgS		*request;
	taskC					*currTask;

	if (strnlen8(devicePath, ZUDIIDX_SERVER_MSG_DEVICEPATH_MAXLEN)
		>= ZUDIIDX_SERVER_MSG_DEVICEPATH_MAXLEN)
		{ return ERROR_LIMIT_OVERFLOWED; };

	currTask = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask();

	request = new zudiIndexMsgS(
		ZUDIIDX_SERVER_LOADDRIVER_REQ, devicePath,
		zudiIndexServer::INDEX_KERNEL);

	if (request == NULL) { return ERROR_MEMORY_NOMEM; };

	return currTask->parent->zasyncStream.send(
		floodplainn.zudiIndexServerTid, request, sizeof(*request),
		ipc::METHOD_BUFFER, 0, privateData);
}

void zudiIndexServer::setNewDeviceActionReq(
	newDeviceActionE action, void *privateData
	)
{
	zudiIndexMsgS		*request;
	taskC			*currTask;

	currTask = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask();

	request = new zudiIndexMsgS(
		ZUDIIDX_SERVER_SET_NEWDEVICE_ACTION_REQ, NULL,
		zudiIndexServer::INDEX_KERNEL, action);

	if (request == NULL) { return; };

	currTask->parent->zasyncStream.send(
		floodplainn.zudiIndexServerTid, request, sizeof(*request),
		ipc::METHOD_BUFFER, 0, privateData);
}

void zudiIndexServer::getNewDeviceActionReq(void *privateData)
{
	zudiIndexMsgS		*request;
	taskC			*currTask;

	currTask = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask();

	request = new zudiIndexMsgS(
		ZUDIIDX_SERVER_GET_NEWDEVICE_ACTION_REQ, NULL,
		zudiIndexServer::INDEX_KERNEL);

	if (request == NULL) { return; };

	currTask->parent->zasyncStream.send(
		floodplainn.zudiIndexServerTid, request, sizeof(*request),
		ipc::METHOD_BUFFER, 0, privateData);
}

void zudiIndexServer::newDeviceInd(
	utf8Char *path, indexE index, void *privateData
	)
{
	zudiIndexMsgS		*request;
	taskC			*currTask;

	if (strnlen8(path, ZUDIIDX_SERVER_MSG_DEVICEPATH_MAXLEN)
		>= ZUDIIDX_SERVER_MSG_DEVICEPATH_MAXLEN)
	{
		printf(ERROR FPLAINN"newDeviceInd: device name too long.\n");
		return;
	};

	currTask = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask();

	request = new zudiIndexMsgS(
		ZUDIIDX_SERVER_NEWDEVICE_IND, path, index);

	if (request == NULL) { return; };

	currTask->parent->zasyncStream.send(
		floodplainn.zudiIndexServerTid, request, sizeof(*request),
		ipc::METHOD_BUFFER, 0, privateData);
}

