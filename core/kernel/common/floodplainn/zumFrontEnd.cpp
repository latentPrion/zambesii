
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
	utf8Char *func, utf8Char *devicePath, fplainn::Zum::sZAsyncMsg::opE op,
	uarch_t extraMemNBytes
	)
{
	fplainn::Zum::sZAsyncMsg	*ret;

	ret = new (extraMemNBytes) sZAsyncMsg(devicePath, op, extraMemNBytes);
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

void fplainn::Zum::enumerateChildrenReq(
	utf8Char *devicePath, udi_enumerate_cb_t *ecb,
	uarch_t flags, void *privateData
	)
{
	HeapObj<sZAsyncMsg>		req;
	Thread				*caller;
	uarch_t				requiredExtraMem;
	sZAsyncMsg			*tmp;
	udi_instance_attr_list_t	*tmp2;

	caller = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	requiredExtraMem =
		ecb->attr_valid_length * sizeof(*ecb->attr_list)
		+ ecb->filter_list_length * sizeof(*ecb->filter_list);

	req = tmp = getNewSZAsyncMsg(
		CC __func__, devicePath, sZAsyncMsg::OP_ENUMERATE_CHILDREN_REQ,
		requiredExtraMem);

	if (req.get() == NULL) { return; };

	/**	EXPLANATION:
	 * Copy the attr and filter lists into the extra mem packed at the
	 * end of the sZASyncMsg.
	 **/
	// "tmp" and "tmp2 "now point to the beginning of the extra mem.
	tmp2 = (udi_instance_attr_list_t *)++tmp;

	// Set request params here.
	req->params.enumerateChildren.flags = flags;

	// Copy filter and list into extra mem.
	if (ecb->attr_valid_length > 0 && ecb->attr_list != NULL)
	{
		memcpy(
			tmp, ecb->attr_list,
			ecb->attr_valid_length * sizeof(*ecb->attr_list));

		ecb->attr_list = (udi_instance_attr_list_t *)tmp;
		tmp2 += ecb->attr_valid_length;
		// "tmp2" now points past the end of the copied attrs in the extra mem.
	};

	// Copy attr and list into extra mem.
	if (ecb->filter_list_length > 0 && ecb->filter_list != NULL)
	{
		memcpy(
			tmp2, ecb->filter_list,
			ecb->filter_list_length * sizeof(*ecb->filter_list));

		ecb->filter_list = (udi_filter_element_t *)tmp2;
	};

	caller->parent->zasyncStream.send(
		serverTid, req.get(),
		sizeof(*req) + req->extraMemNBytes,
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
	uarch_t				requiredExtraMem;
	fplainn::sMovableMemory		*tmp0[2];
	udi_instance_attr_list_t	*tmp;
	udi_filter_element_t		*tmp2;

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

	requiredExtraMem =
		(cb->attr_valid_length * sizeof(*cb->attr_list))
		+ (cb->filter_list_length * sizeof(*cb->filter_list))
		+ (sizeof(fplainn::sMovableMemory) * 2);

	req = getNewSZAsyncMsg(
		CC __func__, devicePath, sZAsyncMsg::OP_ENUMERATE_REQ,
		requiredExtraMem);

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

	/* tmp1[0] = sMovableMem for attr_list.
	 * tmp1[1] = sMovableMem for attr_list.
	 * tmp = extra mem for attr_list.
	 **/
	tmp0[0] = tmp0[1] = (fplainn::sMovableMemory *)(req.get() + 1);
	tmp = (udi_instance_attr_list_t *)(tmp0[0] + 1);

	if (cb->attr_valid_length > 0 && cb->attr_list != NULL)
	{
		::new (tmp0[0]) fplainn::sMovableMemory(
			cb->attr_valid_length * sizeof(*cb->attr_list));

		memcpy(
			tmp, cb->attr_list,
			cb->attr_valid_length * sizeof(*cb->attr_list));

		// Update pointer val
		req->params.enumerate.cb.attr_list =
			(udi_instance_attr_list_t *)tmp;

		// tmp1[1] = sMovableMem for filter_list.
		tmp0[1] = (fplainn::sMovableMemory *)
			(tmp + cb->attr_valid_length);
	};

	// tmp2 = extra mem for filter list.
	tmp2 = (udi_filter_element_t *)(tmp0[1] + 1);

	if (cb->filter_list_length > 0 && cb->filter_list != NULL)
	{
		::new (tmp0[1]) fplainn::sMovableMemory(
			cb->filter_list_length * sizeof(*cb->filter_list));

		memcpy(
			tmp2, cb->filter_list,
			cb->filter_list_length * sizeof(*cb->filter_list));

		// Update pointer val.
		req->params.enumerate.cb.filter_list =
			(udi_filter_element_t *)tmp2;
	};

#if 0
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
#endif

	req->params.enumerate.enumeration_level = enumLevel;

printf(NOTICE"reqmem 0x%p: offsets: 0x%p, 0x%p; 0x%p, 0x%p.\n", req.get(), tmp0[0], tmp, tmp0[1], tmp2);

	caller->parent->zasyncStream.send(
		serverTid, req.get(), sizeof(*req) + requiredExtraMem,
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
