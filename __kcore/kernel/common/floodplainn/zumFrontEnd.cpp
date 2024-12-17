
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
	EnumerateReqMovableObjects	*movableMem;

	caller = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	if (ecb == NULL)
	{
		printf(ERROR ZUM"enumChildrenReq: a udi_enumerate_cb_t arg is "
			"required.\n");
		return;
	}

	if (ecb->attr_list != NULL || ecb->attr_valid_length > 0)
	{
		printf(ERROR ZUM"enumChildrenReq: %d attrs passed in @%p. "
			"Directed enumeration not allowed!",
			ecb->attr_valid_length, ecb->attr_list);
		return;
	};

	requiredExtraMem =
		EnumerateReqMovableObjects::calcMemRequirementsFor(
			ecb->attr_valid_length,
			ecb->filter_list_length);

	req = getNewSZAsyncMsg(
		CC __func__, devicePath, sZAsyncMsg::OP_ENUMERATE_CHILDREN_REQ,
		requiredExtraMem);

	if (req.get() == NULL) { return; };

	memcpy(&req->params.enumerateChildren.cb, ecb, sizeof(*ecb));

	/**	EXPLANATION:
	 * Copy the attr and filter lists into the extra mem packed at the
	 * end of the sZASyncMsg.
	 **/
	movableMem = new (req.get() + 1)
	EnumerateReqMovableObjects(
		ecb->attr_valid_length, ecb->filter_list_length);

	// Set request params here.
	req->params.enumerateChildren.flags = flags;
	req->params.enumerateChildren.deviceIdsHandle = NULL;

	/* We don't copy the attr list because enumerateChildrenReq is not meant
	 * to be used for directed enumeration. We do copy the filter list
	 * because we want to allow the caller to filter which devices it
	 * enumerates.
	 **/

	// Copy filter list into extra mem.
	if (ecb->filter_list_length > 0 && ecb->filter_list != NULL)
	{
		memcpy(
			movableMem->calcFilterList(0, ecb->filter_list_length),
			ecb->filter_list,
			ecb->filter_list_length * sizeof(*ecb->filter_list));

		req->params.enumerateChildren.cb.filter_list =
			movableMem->calcFilterList(0, ecb->filter_list_length);
	};

	caller->parent->zasyncStream.send(
		serverTid, req.get(),
		sizeof(*req) + req->extraMemNBytes,
		ipc::METHOD_BUFFER, 0, privateData);
}

void fplainn::Zum::postManagementCbReq(
	utf8Char *devicePath, udi_enumerate_cb_t *ecb, void *privateData
	)
{
	HeapObj<sZAsyncMsg>		req;
	Thread				*caller;
	uarch_t				requiredExtraMem;
	EnumerateReqMovableObjects	*movableMem;

	caller = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	if (ecb == NULL || ecb->attr_list != NULL || ecb->attr_valid_length > 0)
	{
		printf(ERROR ZUM"postMgmtCb: %d attrs passed in @%p. "
			"Directed enumeration not allowed!",
			ecb->attr_valid_length, ecb->attr_list);
		return;
	};

	requiredExtraMem =
		EnumerateReqMovableObjects::calcMemRequirementsFor(
			ecb->attr_valid_length,
			ecb->filter_list_length);

	req = getNewSZAsyncMsg(
		CC __func__, devicePath, sZAsyncMsg::OP_POST_MANAGEMENT_CB_REQ,
		requiredExtraMem);

	if (req.get() == NULL) { return; };

	memcpy(&req->params.enumerateChildren.cb, ecb, sizeof(*ecb));

	/**	EXPLANATION:
	 * Copy the attr and filter lists into the extra mem packed at the
	 * end of the sZASyncMsg.
	 **/
	movableMem = new (req.get() + 1)
	EnumerateReqMovableObjects(
		ecb->attr_valid_length, ecb->filter_list_length);

	/* We don't copy the attr list because enumerateChildrenReq is not meant
	 * to be used for directed enumeration. We do copy the filter list
	 * because we want to allow the caller to filter which devices it
	 * enumerates.
	 **/

	// Copy filter list into extra mem.
	if (ecb->filter_list_length > 0 && ecb->filter_list != NULL)
	{
		memcpy(
			movableMem->calcFilterList(0, ecb->filter_list_length),
			ecb->filter_list,
			ecb->filter_list_length * sizeof(*ecb->filter_list));

		req->params.enumerateChildren.cb.filter_list =
			movableMem->calcFilterList(0, ecb->filter_list_length);
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
	EnumerateReqMovableObjects	*movableMem;

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

	requiredExtraMem = EnumerateReqMovableObjects::calcMemRequirementsFor(
		cb->attr_valid_length, cb->filter_list_length);

	req = getNewSZAsyncMsg(
		CC __func__, devicePath, sZAsyncMsg::OP_ENUMERATE_REQ,
		requiredExtraMem);

	if (req.get() == NULL) { return; };

	memcpy(&req->params.enumerate.cb, cb, sizeof(*cb));

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

	// Construct offset calc object after sZasyncMsg.
	movableMem = ::new (req.get() + 1) EnumerateReqMovableObjects(
		cb->attr_valid_length, cb->filter_list_length);

	if (cb->attr_valid_length > 0 && cb->attr_list != NULL)
	{
		memcpy(
			movableMem->calcAttrList(cb->attr_valid_length),
			cb->attr_list,
			cb->attr_valid_length * sizeof(*cb->attr_list));

		// Update pointer val
		req->params.enumerate.cb.attr_list = movableMem->calcAttrList(
			cb->attr_valid_length);
	}
	else {
		req->params.enumerate.cb.attr_list = NULL;
	};

	if (cb->filter_list_length > 0 && cb->filter_list != NULL)
	{
		memcpy(
			movableMem->calcFilterList(
				cb->attr_valid_length, cb->filter_list_length),
			cb->filter_list,
			cb->filter_list_length * sizeof(*cb->filter_list));

		// Update pointer val.
		req->params.enumerate.cb.filter_list = movableMem
			->calcFilterList(
				cb->attr_valid_length, cb->filter_list_length);
	}
	else {
		req->params.enumerate.cb.filter_list = NULL;
	};

	req->params.enumerate.enumeration_level = enumLevel;

	caller->parent->zasyncStream.send(
		serverTid, req.get(), sizeof(*req) + requiredExtraMem,
		ipc::METHOD_BUFFER, 0, privateData);
}

void fplainn::Zum::deviceManagementReq(
	utf8Char *devicePath, udi_ubit8_t op, ubit16 parentId, void *privateData
	)
{
	HeapObj<sZAsyncMsg>		req;
	Thread				*caller;

	caller = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	req = getNewSZAsyncMsg(
		CC __func__, devicePath, sZAsyncMsg::OP_DEVMGMT_REQ);

	if (req.get() == NULL) { return; };

	req->params.devmgmt.mgmt_op = op;
	req->params.devmgmt.parent_ID = parentId;

	caller->parent->zasyncStream.send(
		serverTid, req.get(), sizeof(*req),
		ipc::METHOD_BUFFER, 0, privateData);
}

void fplainn::Zum::finalCleanupReq(utf8Char *devicePath, void *privateData)
{
	HeapObj<sZAsyncMsg>		req;
	Thread				*caller;

	caller = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	req = getNewSZAsyncMsg(
		CC __func__, devicePath, sZAsyncMsg::OP_FINAL_CLEANUP_REQ);

	if (req.get() == NULL) { return; };

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
