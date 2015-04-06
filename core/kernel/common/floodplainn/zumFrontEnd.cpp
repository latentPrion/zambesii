
#include <__kstdlib/__kcxxlib/memory>
#include <__kstdlib/__kclib/string.h>
#include <kernel/common/thread.h>
#include <kernel/common/process.h>
#include <kernel/common/floodplainn/zum.h>
#include <kernel/common/floodplainn/movableMemory.h>
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

void fplainn::Zum::enumerateReq(
	utf8Char *devicePath, udi_ubit8_t enumLevel,
	udi_enumerate_cb_t *cb, void *privateData
	)
{
	HeapObj<sZAsyncMsg>		req;
	Thread				*caller;

	if (cb == NULL) {
		printf(ERROR ZUM"enumerateReq: cb arg is invalid.\n");
		return;
	};

	if (cb->attr_valid_length > 0 && cb->attr_list == NULL) {
		printf(ERROR ZUM"enumerateReq: cb arg's attr_list is invalid.\n");
		return;
	};

	if (cb->filter_list_length > 0 && cb->filter_list == NULL) {
		printf(ERROR ZUM"enumerateReq: cb arg's filter_list is invalid.\n");
		return;
	};

	caller = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	req = getNewSZAsyncMsg(
		CC __func__, devicePath, sZAsyncMsg::OP_ENUMERATE_REQ);

	if (req.get() == NULL) { return; };

	req->params.enumerate.cb.child_ID = cb->child_ID;
	req->params.enumerate.cb.parent_ID = cb->parent_ID;

	req->params.enumerate.cb.attr_valid_length = cb->attr_valid_length;
	req->params.enumerate.cb.filter_list_length = cb->filter_list_length;

	/**	EXPLaNaTION:
	 * We alloc space to hold the attr and filter lists, and pass them to
	 * the ZUM server. The ZUM server will deallocate them when it's done
	 * with them. See, we can't just pass the filter_list and attr_list we
	 * were given because they could be in userspace. That would cause them
	 * to be inaccessible to the ZUM server since it's in a different
	 * addrspace.
	 **/
	req->params.enumerate.cb.attr_list = NULL;
	req->params.enumerate.cb.filter_list = NULL;

	if (cb->attr_valid_length > 0 && cb->attr_list != NULL)
	{
		const uarch_t			listNBytes =
			sizeof(*cb->attr_list) * cb->attr_valid_length;

		/* If we're going to pass these attrs to sChannelMsg::send() as
		 * inline pointers, we need to give them MEM_MOVaBLE headers.
		 **/
		req->params.enumerate.cb.attr_list = new
			((new (listNBytes) fplainn::sMovableMemory(listNBytes)) + 1)
			udi_instance_attr_list_t;

		if (req->params.enumerate.cb.attr_list == NULL)
		{
			printf(ERROR ZUM"enumerateReq: failed to alloc mem for "
				"attr list.\n");
			return;
		};

		memcpy(
			req->params.enumerate.cb.attr_list, cb->attr_list,
			listNBytes);
	};

	if (cb->filter_list_length > 0 && cb->filter_list != NULL)
	{
		const uarch_t			listNBytes =
			sizeof(*cb->filter_list) * cb->filter_list_length;

		/* If we're going to pass these filters to sChannelMsg::send() as
		 * inline pointers, we need to give them MEM_MOVaBLE headers.
		 **/
		req->params.enumerate.cb.filter_list = new
			((new (listNBytes) fplainn::sMovableMemory(listNBytes)) + 1)
			udi_filter_element_t;

		if (req->params.enumerate.cb.filter_list == NULL)
		{
			printf(ERROR ZUM"enumerateReq: failed to alloc mem for "
				"filter list.\n");
			return;
		};

		memcpy(
			const_cast<udi_filter_element_t *>(req->params.enumerate.cb.filter_list),
			cb->filter_list,
			listNBytes);
	};

	req->params.enumerate.enumeration_level = enumLevel;

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
