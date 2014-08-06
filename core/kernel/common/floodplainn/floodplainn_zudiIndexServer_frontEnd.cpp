
#include <__kstdlib/__kcxxlib/memory>
#include <kernel/common/thread.h>
#include <kernel/common/process.h>
#include <kernel/common/zudiIndexServer.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/floodplainn/floodplainn.h>


error_t zuiServer::detectDriverReq(
	utf8Char *devicePath, indexE index,
	void *privateData, ubit32 flags
	)
{
	HeapObj<zuiServer::sIndexMsg>	request;
	Thread				*currThread;

	if (strnlen8(devicePath, FVFS_PATH_MAXLEN) >= FVFS_PATH_MAXLEN)
		{ return ERROR_LIMIT_OVERFLOWED; };

	currThread = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	// Allocate and queue the new request.
	request = new zuiServer::sIndexMsg(
		ZUISERVER_DETECTDRIVER_REQ, devicePath, index);

	if (request == NULL) { return ERROR_MEMORY_NOMEM; };

	return currThread->parent->zasyncStream.send(
		floodplainn.zudiIndexServerTid, request.get(), sizeof(*request),
		ipc::METHOD_BUFFER, flags, privateData);
}

error_t zuiServer::loadDriverReq(utf8Char *devicePath, void *privateData)
{
	HeapObj<zuiServer::sIndexMsg>	request;
	Thread				*currThread;

	if (strnlen8(devicePath, FVFS_PATH_MAXLEN) >= FVFS_PATH_MAXLEN)
		{ return ERROR_LIMIT_OVERFLOWED; };

	currThread = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	request = new zuiServer::sIndexMsg(
		ZUISERVER_LOADDRIVER_REQ, devicePath,
		zuiServer::INDEX_KERNEL);

	if (request == NULL) { return ERROR_MEMORY_NOMEM; };

	return currThread->parent->zasyncStream.send(
		floodplainn.zudiIndexServerTid, request.get(), sizeof(*request),
		ipc::METHOD_BUFFER, 0, privateData);
}

void zuiServer::loadDriverRequirementsReq(void *privateData)
{
	HeapObj<zuiServer::sIndexMsg>	request;
	Thread				*currThread;

	currThread = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();
	if (currThread->parent->getType() != ProcessStream::DRIVER)
		{ return; };

	request = new zuiServer::sIndexMsg(
		ZUISERVER_LOAD_REQUIREMENTS_REQ,
		currThread->parent->fullName,
		zuiServer::INDEX_KERNEL);

	if (request == NULL) { return; };

	currThread->parent->zasyncStream.send(
		floodplainn.zudiIndexServerTid, request.get(), sizeof(*request),
		ipc::METHOD_BUFFER, 0, privateData);
}

void zuiServer::setNewDeviceActionReq(
	newDeviceActionE action, void *privateData
	)
{
	HeapObj<zuiServer::sIndexMsg>	request;
	Thread			*currThread;

	currThread = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	request = new zuiServer::sIndexMsg(
		ZUISERVER_SET_NEWDEVICE_ACTION_REQ, NULL,
		zuiServer::INDEX_KERNEL, action);

	if (request == NULL) { return; };

	currThread->parent->zasyncStream.send(
		floodplainn.zudiIndexServerTid, request.get(), sizeof(*request),
		ipc::METHOD_BUFFER, 0, privateData);
}

void zuiServer::getNewDeviceActionReq(void *privateData)
{
	HeapObj<zuiServer::sIndexMsg>	request;
	Thread				*currThread;

	currThread = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	request = new zuiServer::sIndexMsg(
		ZUISERVER_GET_NEWDEVICE_ACTION_REQ, NULL,
		zuiServer::INDEX_KERNEL);

	if (request == NULL) { return; };

	currThread->parent->zasyncStream.send(
		floodplainn.zudiIndexServerTid, request.get(), sizeof(*request),
		ipc::METHOD_BUFFER, 0, privateData);
}

void zuiServer::newDeviceInd(
	utf8Char *path, indexE index, void *privateData
	)
{
	HeapObj<zuiServer::sIndexMsg>	request;
	Thread				*currThread;

	if (strnlen8(path, FVFS_PATH_MAXLEN) >= FVFS_PATH_MAXLEN)
	{
		printf(ERROR FPLAINN"newDeviceInd: device name too long.\n");
		return;
	};

	currThread = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	request = new zuiServer::sIndexMsg(
		ZUISERVER_NEWDEVICE_IND, path, index);

	if (request == NULL) { return; };

	currThread->parent->zasyncStream.send(
		floodplainn.zudiIndexServerTid, request.get(), sizeof(*request),
		ipc::METHOD_BUFFER, 0, privateData);
}

