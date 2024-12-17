
#include <__kstdlib/__kcxxlib/memory>
#include <kernel/common/thread.h>
#include <kernel/common/process.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/floodplainn/floodplainn.h>
#include <kernel/common/floodplainn/zui.h>


error_t fplainn::Zui::initialize(void)
{
	error_t		ret;

	ret = processTrib.__kgetStream()->spawnThread(
		&main, NULL,
		NULL, Thread::ROUND_ROBIN,
		0, 0, &server);

	if (ret != ERROR_SUCCESS)
	{
		printf(FATAL ZUI"Failed to spawn server thread.\n");
		return ret;
	};

	setServerTid(server->getFullId());
	taskTrib.yield();
	return ERROR_SUCCESS;
}

error_t fplainn::Zui::detectDriverReq(
	utf8Char *devicePath, indexE index,
	void *privateData, ubit32 flags
	)
{
	HeapObj<sIndexZAsyncMsg>	request;
	Thread				*currThread;

	if (strnlen8(devicePath, FVFS_PATH_MAXLEN) >= FVFS_PATH_MAXLEN)
		{ return ERROR_LIMIT_OVERFLOWED; };

	currThread = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	// Allocate and queue the new request.
	request = new sIndexZAsyncMsg(
		MSGSTREAM_ZUI_DETECTDRIVER_REQ, devicePath, index);

	if (request == NULL) { return ERROR_MEMORY_NOMEM; };

	return currThread->parent->zasyncStream.send(
		floodplainn.zui.serverTid, request.get(), sizeof(*request),
		ipc::METHOD_BUFFER, flags, privateData);
}

error_t fplainn::Zui::loadDriverReq(utf8Char *devicePath, void *privateData)
{
	HeapObj<sIndexZAsyncMsg>	request;
	Thread				*currThread;

	if (strnlen8(devicePath, FVFS_PATH_MAXLEN) >= FVFS_PATH_MAXLEN)
		{ return ERROR_LIMIT_OVERFLOWED; };

	currThread = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	request = new sIndexZAsyncMsg(
		MSGSTREAM_ZUI_LOADDRIVER_REQ, devicePath,
		fplainn::Zui::INDEX_KERNEL);

	if (request == NULL) { return ERROR_MEMORY_NOMEM; };

	return currThread->parent->zasyncStream.send(
		floodplainn.zui.serverTid, request.get(), sizeof(*request),
		ipc::METHOD_BUFFER, 0, privateData);
}

void fplainn::Zui::loadDriverRequirementsReq(void *privateData)
{
	HeapObj<sIndexZAsyncMsg>	request;
	Thread				*currThread;

	currThread = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();
	if (currThread->parent->getType() != ProcessStream::DRIVER)
		{ return; };

	request = new sIndexZAsyncMsg(
		MSGSTREAM_ZUI_LOAD_REQUIREMENTS_REQ,
		currThread->parent->fullName,
		fplainn::Zui::INDEX_KERNEL);

	if (request == NULL) { return; };

	currThread->parent->zasyncStream.send(
		floodplainn.zui.serverTid, request.get(), sizeof(*request),
		ipc::METHOD_BUFFER, 0, privateData);
}

void fplainn::Zui::setNewDeviceActionReq(
	newDeviceActionE action, void *privateData
	)
{
	HeapObj<sIndexZAsyncMsg>	request;
	Thread			*currThread;

	currThread = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	request = new sIndexZAsyncMsg(
		MSGSTREAM_ZUI_SET_NEWDEVICE_ACTION_REQ, NULL,
		fplainn::Zui::INDEX_KERNEL, action);

	if (request == NULL) { return; };

	currThread->parent->zasyncStream.send(
		floodplainn.zui.serverTid, request.get(), sizeof(*request),
		ipc::METHOD_BUFFER, 0, privateData);
}

void fplainn::Zui::getNewDeviceActionReq(void *privateData)
{
	HeapObj<sIndexZAsyncMsg>	request;
	Thread				*currThread;

	currThread = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	request = new sIndexZAsyncMsg(
		MSGSTREAM_ZUI_GET_NEWDEVICE_ACTION_REQ, NULL,
		fplainn::Zui::INDEX_KERNEL);

	if (request == NULL) { return; };

	currThread->parent->zasyncStream.send(
		floodplainn.zui.serverTid, request.get(), sizeof(*request),
		ipc::METHOD_BUFFER, 0, privateData);
}

void fplainn::Zui::newDeviceInd(
	utf8Char *path, indexE index, void *privateData
	)
{
	HeapObj<sIndexZAsyncMsg>	request;
	Thread				*currThread;

	if (strnlen8(path, FVFS_PATH_MAXLEN) >= FVFS_PATH_MAXLEN)
	{
		printf(ERROR FPLAINN"newDeviceInd: device name too long.\n");
		return;
	};

	currThread = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	request = new sIndexZAsyncMsg(
		MSGSTREAM_ZUI_NEWDEVICE_IND, path, index);

	if (request == NULL) { return; };

	currThread->parent->zasyncStream.send(
		floodplainn.zui.serverTid, request.get(), sizeof(*request),
		ipc::METHOD_BUFFER, 0, privateData);
}

