
#include <__kstdlib/__kcxxlib/memory>
#include <__kstdlib/__kclib/string.h>
#include <kernel/common/thread.h>
#include <kernel/common/process.h>
#include <kernel/common/floodplainn/zum.h>
#include <kernel/common/floodplainn/movableMemory.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/taskTrib/taskTrib.h>


/* Unmashaling methods for enumerateReq/Ack.
 ******************************************************************************/

void fplainn::Zum::EnumerateReqMovableMemMarshaller::marshalAttrList(
	const udi_instance_attr_list_t *const srcAttrList,
	uarch_t nAttrs)
{
	if (nAttrs > 0 && srcAttrList != NULL)
	{
		memcpy(
			calcAttrListPtr(nAttrs),
			srcAttrList,
			sizeof(*srcAttrList) * nAttrs);
	}
}

void fplainn::Zum::EnumerateReqMovableMemMarshaller::marshalFilterList(
	const udi_filter_element_t *const srcFilterList,
	uarch_t nAttrs, uarch_t nFilters)
{
	if (nFilters > 0 && srcFilterList != NULL)
	{
		memcpy(
			calcFilterListPtr(nAttrs, nFilters),
			srcFilterList,
			sizeof(*srcFilterList) * nFilters);
	}
}

void fplainn::Zum::EnumerateReqMovableMemMarshaller::unmarshalAttrList(
	udi_instance_attr_list_t *destAttrList,
	uarch_t nAttrs) const
{
	if (nAttrs > 0 && destAttrList != NULL)
	{
		memcpy(
			destAttrList,
			calcAttrListPtr(nAttrs),
			sizeof(*destAttrList) * nAttrs);
	}
}

void fplainn::Zum::EnumerateReqMovableMemMarshaller::unmarshalFilterList(
	udi_filter_element_t *destFilterList,
	uarch_t nAttrs, uarch_t nFilters) const
{
	if (nFilters > 0 && destFilterList != NULL)
	{
		memcpy(
			destFilterList,
			calcFilterListPtr(nAttrs, nFilters),
			sizeof(*destFilterList) * nFilters);
	}
}

void fplainn::Zum::EnumerateReqMovableMemMarshaller::marshal(
	const udi_instance_attr_list_t *const srcAttrList, uarch_t nAttrs,
	const udi_filter_element_t *const srcFilterList, uarch_t nFilters)
{
	// Marshal attribute list if provided
	if (nAttrs > 0 && srcAttrList != NULL) {
		marshalAttrList(srcAttrList, nAttrs);
	}

	// Marshal filter list if provided
	if (nFilters > 0 && srcFilterList != NULL) {
		marshalFilterList(srcFilterList, nAttrs, nFilters);
	}
}

void fplainn::Zum::EnumerateReqMovableMemMarshaller::unmarshal(
	udi_instance_attr_list_t *destAttrList, uarch_t nAttrs,
	udi_filter_element_t *destFilterList, uarch_t nFilters) const
{
	// Unmarshal attribute list if requested
	if (nAttrs > 0 && destAttrList != NULL) {
		unmarshalAttrList(destAttrList, nAttrs);
	}

	// Unmarshal filter list if requested
	if (nFilters > 0 && destFilterList != NULL) {
		unmarshalFilterList(destFilterList, nAttrs, nFilters);
	}
}

/* Server methods.
 ******************************************************************************/

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

fplainn::Zum::sZumServerMsg *fplainn::Zum::createServerRequestMsg(
	utf8Char *func, utf8Char *devicePath, fplainn::Zum::sZumServerMsg::opE op,
	uarch_t extraMemNBytes
	)
{
	fplainn::Zum::sZumServerMsg	*ret;

	ret = new (extraMemNBytes) sZumServerMsg(devicePath, op, extraMemNBytes);
	if (ret == NULL)
	{
		printf(ERROR ZUM"%s %s: Failed to alloc ZAsync req "
			"object.\n",
			func, devicePath);
		return NULL;
	}
	
	return ret;
}

error_t fplainn::Zum::unmarshalEnumerateAckAttrsAndFilters(
	udi_enumerate_cb_t *enumCb,
	udi_instance_attr_list_t *attrList,
	udi_filter_element_t *filterList)
{
	EnumerateReqMovableMemMarshaller	*movableMem;
	fplainn::sMovableMemory			*tmp;
	uarch_t					nAttrs, nFilters;

	nAttrs = enumCb->attr_valid_length;
	nFilters = enumCb->filter_list_length;

	if (nAttrs == 0 && nFilters == 0)
		{ return ERROR_SUCCESS; }

	if (attrList == NULL && filterList == NULL)
		{ return ERROR_INVALID_ARG_VAL; }

	if (attrList == NULL && nAttrs > 0)
		{ return ERROR_INVALID_ARG_VAL; }

	if (filterList == NULL && nFilters > 0)
		{ return ERROR_INVALID_ARG_VAL; }

	// Get the marshaller object
	if (nFilters > 0 && filterList != NULL) {
		tmp = (fplainn::sMovableMemory *)enumCb->filter_list;
		movableMem = (EnumerateReqMovableMemMarshaller *)(tmp - 1);
	} else if (nAttrs > 0 && attrList != NULL) {
		tmp = (fplainn::sMovableMemory *)enumCb->attr_list;
		movableMem = (EnumerateReqMovableMemMarshaller *)(tmp - 1);
	} else {
		return ERROR_SUCCESS;
	}

	// Use the unmarshal wrapper method to handle both attr and filter lists
	movableMem->unmarshal(attrList, nAttrs, filterList, nFilters);
	// Delete the movable memory object
	delete movableMem;

	return ERROR_SUCCESS;
}

void fplainn::Zum::startDeviceReq(
	utf8Char *devicePath, udi_ubit8_t resource_level, void *privateData
	)
{
	HeapObj<sZumServerMsg>		req;
	Thread				*caller;

	caller = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	req = createServerRequestMsg(
		CC __func__, devicePath, sZumServerMsg::OP_START_REQ);

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
	HeapObj<sZumServerMsg>		req;
	Thread				*caller;
	uarch_t				requiredExtraMem;
	EnumerateReqMovableMemMarshaller	*movableMem;

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
		EnumerateReqMovableMemMarshaller::calcMemRequirementsFor(
			ecb->attr_valid_length,
			ecb->filter_list_length);

	req = createServerRequestMsg(
		CC __func__, devicePath, sZumServerMsg::OP_ENUMERATE_CHILDREN_REQ,
		requiredExtraMem);

	if (req.get() == NULL) { return; };

	memcpy(&req->params.enumerateChildren.cb, ecb, sizeof(*ecb));

	/**	EXPLANATION:
	 * Use EnumerateReqMovableMemMarshaller to construct the MovableMem
	 * headers directly in the extra memory allocated after the
	 * sZumServerMsg.
	 *
	 * Then copy the attr and filter lists into the extra mem.
	 **/
	movableMem = new (req.get() + 1) EnumerateReqMovableMemMarshaller(
			ecb->attr_valid_length, ecb->filter_list_length);

	// Set request params here.
	req->params.enumerateChildren.flags = flags;
	req->params.enumerateChildren.deviceIdsHandle = NULL;

	/* We don't copy the attr list because enumerateChildrenReq is not meant
	 * to be used for directed enumeration. We do copy the filter list
	 * because we want to allow the caller to filter which devices it
	 * enumerates.
	 **/

	if (ecb->filter_list_length > 0 && ecb->filter_list != NULL)
	{
		// Marshal only the filter list (attr_list is NULL)
		movableMem->marshal(
			NULL, 0, ecb->filter_list, ecb->filter_list_length);

		// Update filter list pointer in the request
		req->params.enumerateChildren.cb.filter_list =
			movableMem->calcFilterListPtr(0, ecb->filter_list_length);
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
	HeapObj<sZumServerMsg>		req;
	Thread				*caller;
	uarch_t				requiredExtraMem;
	EnumerateReqMovableMemMarshaller	*movableMem;

	caller = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	if (ecb == NULL || ecb->attr_list != NULL || ecb->attr_valid_length > 0)
	{
		printf(ERROR ZUM"postMgmtCb: %d attrs passed in @%p. "
			"Directed enumeration not allowed!",
			ecb->attr_valid_length, ecb->attr_list);
		return;
	};

	requiredExtraMem =
		EnumerateReqMovableMemMarshaller::calcMemRequirementsFor(
			ecb->attr_valid_length,
			ecb->filter_list_length);

	req = createServerRequestMsg(
		CC __func__, devicePath, sZumServerMsg::OP_POST_MANAGEMENT_CB_REQ,
		requiredExtraMem);

	if (req.get() == NULL) { return; };

	memcpy(&req->params.enumerateChildren.cb, ecb, sizeof(*ecb));

	/**	EXPLANATION:
	 * Use EnumerateReqMovableMemMarshaller to construct the MovableMem
	 * headers directly in the extra memory allocated after the
	 * sZumServerMsg.
	 *
	 * Then copy the attr and filter lists into the extra mem.
	 **/
	movableMem = new (req.get() + 1) EnumerateReqMovableMemMarshaller(
		ecb->attr_valid_length, ecb->filter_list_length);

	/* We don't copy the attr list because enumerateChildrenReq is not meant
	 * to be used for directed enumeration. We do copy the filter list
	 * because we want to allow the caller to filter which devices it
	 * enumerates.
	 **/

	if (ecb->filter_list_length > 0 && ecb->filter_list != NULL)
	{
		// Marshal only the filter list (attr_list is NULL)
		movableMem->marshal(NULL, 0, ecb->filter_list, ecb->filter_list_length);

		// Update filter list pointer in the request
		req->params.enumerateChildren.cb.filter_list =
			movableMem->calcFilterListPtr(0, ecb->filter_list_length);
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
	HeapObj<sZumServerMsg>		req;
	Thread				*caller;

	caller = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	req = createServerRequestMsg(
		CC __func__, devicePath, sZumServerMsg::OP_USAGE_IND);

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
	HeapObj<sZumServerMsg>		req;
	Thread				*caller;
	uarch_t				requiredExtraMem;
	EnumerateReqMovableMemMarshaller	*movableMem;

	if (cb == NULL)
	{
		printf(ERROR ZUM"enumerateReq: cb arg is invalid.\n");
		return;
	};

	if (cb->attr_valid_length > 0 && cb->attr_list == NULL)
	{
		printf(ERROR ZUM"enumerateReq: cb arg's attr_list is invalid.\n");
		return;
	};

	if (cb->filter_list_length > 0 && cb->filter_list == NULL) {
		printf(ERROR ZUM"enumerateReq: cb arg's filter_list is invalid.\n");
		return;
	};

	caller = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	requiredExtraMem = EnumerateReqMovableMemMarshaller::calcMemRequirementsFor(
		cb->attr_valid_length, cb->filter_list_length);

	req = createServerRequestMsg(
		CC __func__, devicePath, sZumServerMsg::OP_ENUMERATE_REQ,
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

	/**	EXPLANATION:
	 * Use EnumerateReqMovableMemMarshaller to construct the MovableMem
	 * headers directly in the extra memory allocated after the
	 * sZumServerMsg.
	 *
	 * Then copy the attr and filter lists into the extra mem.
	 **/
	movableMem = new (req.get() + 1) EnumerateReqMovableMemMarshaller(
		cb->attr_valid_length, cb->filter_list_length);

	// Use the marshal wrapper method to handle both attr and filter lists
	movableMem->marshal(cb->attr_list, cb->attr_valid_length,
		cb->filter_list, cb->filter_list_length);

	if (cb->attr_valid_length > 0 && cb->attr_list != NULL)
	{
		// Update pointer val
		req->params.enumerate.cb.attr_list = movableMem->calcAttrListPtr(
			cb->attr_valid_length);
	}
	else {
		req->params.enumerate.cb.attr_list = NULL;
	};

	if (cb->filter_list_length > 0 && cb->filter_list != NULL)
	{
		// Update pointer val.
		req->params.enumerate.cb.filter_list = movableMem
			->calcFilterListPtr(
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
	HeapObj<sZumServerMsg>		req;
	Thread				*caller;

	caller = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	req = createServerRequestMsg(
		CC __func__, devicePath, sZumServerMsg::OP_DEVMGMT_REQ);

	if (req.get() == NULL) { return; };

	req->params.devmgmt.mgmt_op = op;
	req->params.devmgmt.parent_ID = parentId;

	caller->parent->zasyncStream.send(
		serverTid, req.get(), sizeof(*req),
		ipc::METHOD_BUFFER, 0, privateData);
}

void fplainn::Zum::finalCleanupReq(utf8Char *devicePath, void *privateData)
{
	HeapObj<sZumServerMsg>		req;
	Thread				*caller;

	caller = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	req = createServerRequestMsg(
		CC __func__, devicePath, sZumServerMsg::OP_FINAL_CLEANUP_REQ);

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
	HeapObj<sZumServerMsg>		req;
	Thread				*caller;

	caller = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	req = createServerRequestMsg(
		CC __func__, devicePath, sZumServerMsg::OP_CHANNEL_EVENT_IND);

	if (req.get() == NULL) { return; };

	req->params.channel_event.endpoint = endp;
	req->params.channel_event.cb.event = event;
	memcpy(&req->params.channel_event.cb.params, params, sizeof(*params));

	caller->parent->zasyncStream.send(
		serverTid, req.get(), sizeof(*req),
		ipc::METHOD_BUFFER, 0, privateData);
}
