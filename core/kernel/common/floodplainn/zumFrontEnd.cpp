
#include <__kstdlib/__kcxxlib/memory>
#include <__kstdlib/__kclib/string.h>
#include <kernel/common/thread.h>
#include <kernel/common/process.h>
#include <kernel/common/floodplainn/zum.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/taskTrib/taskTrib.h>


error_t fplainn::Zum::initialize(void)
{
	error_t		ret;

	ret = processTrib.__kgetStream()->spawnThread(
		&main, NULL,
		NULL, Thread::ROUND_ROBIN,
		0, 0, &server);

	if (ret != ERROR_SUCCESS)
	{
		printf(FATAL ZUM"Failed to spawn server thread.\n");
		return ret;
	};

	setServerTid(server->getFullId());
	taskTrib.yield();
	return ERROR_SUCCESS;
}

fplainn::Zum::sZAsyncMsg *fplainn::Zum::getNewSZAsyncMsg(
	utf8Char *func, utf8Char *devicePath, fplainn::Zum::sZAsyncMsg::opE op
	)
{
	fplainn::Zum::sZAsyncMsg	*ret;

	ret = new sZAsyncMsg(devicePath, op);
	if (ret == NULL)
	{
		printf(ERROR ZUM"%s %s: Failed to alloc ZAsync req "
			"object.\n",
			func, devicePath);
	};

	return ret;
}

void fplainn::Zum::startDeviceReq(
	utf8Char *devicePath, udi_ubit8_t resource_level, void *privateData
	)
{
	HeapObj<sZAsyncMsg>		req;
	Thread				*caller;

	caller = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	req = getNewSZAsyncMsg(
		CC __func__, devicePath, sZAsyncMsg::OP_START_REQ);

	if (req.get() == NULL) { return; };

	req->params.usage.cb.meta_idx = 0;
	req->params.usage.cb.trace_mask = 0;
	req->params.usage.resource_level = resource_level;

	caller->parent->zasyncStream.send(
		serverTid, req.get(), sizeof(*req),
		ipc::METHOD_BUFFER, 0, privateData);
}

void fplainn::Zum::usageInd(
	utf8Char *devicePath, udi_ubit8_t resource_level, void *privateData
	)
{
	HeapObj<sZAsyncMsg>		req;
	Thread				*caller;

	caller = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	req = getNewSZAsyncMsg(
		CC __func__, devicePath, sZAsyncMsg::OP_USAGE_IND);

	if (req.get() == NULL) { return; };

	req->params.usage.cb.meta_idx = 0;
	req->params.usage.cb.trace_mask = 0;
	req->params.usage.resource_level = resource_level;

	caller->parent->zasyncStream.send(
		serverTid, req.get(), sizeof(*req),
		ipc::METHOD_BUFFER, 0, privateData);
}

void fplainn::Zum::channelEventInd(
	utf8Char *devicePath, fplainn::Endpoint *endp,
	udi_ubit8_t event, eventIndU *params, void *privateData
	)
{
	HeapObj<sZAsyncMsg>		req;
	Thread				*caller;

	caller = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	req = getNewSZAsyncMsg(
		CC __func__, devicePath, sZAsyncMsg::OP_CHANNEL_EVENT_IND);

	if (req.get() == NULL) { return; };

	req->params.channel_event.endpoint = endp;
	req->params.channel_event.cb.event = event;
	memcpy(&req->params.channel_event.cb.params, params, sizeof(*params));

	caller->parent->zasyncStream.send(
		serverTid, req.get(), sizeof(*req),
		ipc::METHOD_BUFFER, 0, privateData);
}
