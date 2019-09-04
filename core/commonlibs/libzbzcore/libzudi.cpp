
#define UDI_VERSION	0x101
#include <udi.h>
#include <__kstdlib/__kclib/string.h>
#include <commonlibs/libzbzcore/libzudi.h>
#include <commonlibs/libzbzcore/libzbzcore.h>
#include <kernel/common/thread.h>
#include <kernel/common/process.h>
#include <kernel/common/panic.h>
#include <kernel/common/floodplainn/floodplainn.h>
#include <kernel/common/floodplainn/movableMemory.h>
#include <kernel/common/floodplainn/initInfo.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


char *udi_strcpy(char *s0, const char *s1)
{
	return (char *)strcpy8(CC s0, CC s1);
}

lzudi::sEndpointContext::sEndpointContext(
	fplainn::Endpoint *__kendp,
	utf8Char *metaName,
	udi_mei_init_t *metaInfo, udi_index_t opsIdx,
	udi_index_t bindCbIndex,
	void *channel_context
	)
:
__kendpoint(__kendp), metaInfo(metaInfo), channel_context(channel_context),
bindCbIndex(bindCbIndex)
{
	anchor(metaName, metaInfo, opsIdx);
}

void lzudi::sEndpointContext::anchor(
	utf8Char *metaName, udi_mei_init_t *metaInfo, udi_index_t ops_idx
)
{
	if (metaName != NULL)
	{
		strncpy8(
			this->metaName, metaName,
			ZUI_DRIVER_METALANGUAGE_MAXLEN);

		this->metaName[ZUI_DRIVER_METALANGUAGE_MAXLEN - 1] = '\0';
	};

	fplainn::MetaInit		metaInit(metaInfo);

	if (metaInfo != NULL) {
		opsVectorTemplate = metaInit.getOpsVectorTemplate(ops_idx);
	};
}

void lzudi::sEndpointContext::dump(void)
{
	printf(NOTICE"endpointCtxt: %p: metaInfo %p, metaName %s.\n"
		"\t__kendp %p, chan_ctxt %p, opsVecTemplate %p.\n",
		this, metaInfo, metaName, __kendpoint, channel_context,
		opsVectorTemplate);
}

static lzudi::sRegion *getRegionMetadataByIndex(udi_index_t index)
{
	List<lzudi::sRegion>		*metadataList;
	List<lzudi::sRegion>::Iterator	mlIt;

	metadataList = &cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentThread()->getRegion()->parent->regionLocalMetadata;

	// Really hate that we have to do this loop, but no way around it.
	mlIt = metadataList->begin();
	for (; mlIt != metadataList->end(); ++mlIt)
	{
		lzudi::sRegion		*tmp = *mlIt;

		if (tmp->index != index) { continue; };
		// Found it.
		return tmp;
	};

	return NULL;
}

void udi_channel_spawn(
	udi_channel_spawn_call_t *callback,
	udi_cb_t *gcb,
	udi_channel_t channel,
	udi_index_t spawn_idx,
	udi_index_t ops_idx,
	void *channel_context
	)
{
	lzudi::sRegion			*r=NULL;
	Thread				*thread;
	lzudi::sEndpointContext		*originEndp, *newEndp;
	fplainn::Endpoint		*__kendp;
	error_t				err;
	__klzbzcore::driver::CachedInfo	*drvInfoCache;
	udi_ops_init_t			*opsInitEntry=NULL;

	if (channel == NULL || gcb == NULL || callback == NULL) {
		callback(gcb, NULL); return;
	};

	originEndp = (lzudi::sEndpointContext *)channel;
	thread = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();
	drvInfoCache = thread->parent->getDriverInstance()->getCachedInfo();

	r = getRegionMetadataByIndex(thread->getRegion()->index);
	if (r == NULL) {
		callback(gcb, NULL); return;
	};

	// Now proceed to make the calls as needed.
	fplainn::DriverInit		drvInitParser(drvInfoCache->initInfo);

	if (ops_idx != 0)
	{
		opsInitEntry = drvInitParser.getOpsInit(ops_idx);
		if (opsInitEntry == NULL) {
			callback(gcb, NULL); return;
		};
	};

	// Does the origin channel exist, and is it anchored to this region?
	if (!r->findEndpointContext(originEndp)) {
		callback(gcb, NULL); return;
	};

	/* FIXME: We can spawn non-bind endpoints without a metaName, and it'll
	 * be fine; they are only functionally required for bind channels. But
	 * eventually we still should do this since it doesn't hurt and it adds
	 * an extra bit of validation checking. For now we pass NULL to the
	 * "metaName" argument.
	 **/
	err = floodplainn.zudi.spawnEndpoint(
		originEndp->__kendpoint, NULL, spawn_idx,
		((ops_idx == 0) ? NULL : opsInitEntry->ops_vector), NULL,
		&__kendp);

	if (err != ERROR_SUCCESS)
	{
		printf(ERROR LZUDI"udi_chan_sp: spawnEndpoint call failed.\n");
		callback(gcb, NULL);
		return;
	};

	// Is there already an ongoing spawn with this spawn_idx?
	newEndp = r->getEndpointContextBy__kendpoint(__kendp);
	if (newEndp != NULL) {
		// Callback with success, because endpoint already exists.
		callback(gcb, newEndp); return;
	};

	/* Else, this channel is newly being spawned. We need to create a new
	 * sEndpointContext for it. Get the metalanguage lib and name.
	 **/
	if (ops_idx != 0)
	{
		__klzbzcore::driver::CachedInfo::sMetaDescriptor      *metaDesc;

		metaDesc = drvInfoCache->getMetaDescriptor(
			opsInitEntry->meta_idx);

		newEndp = new lzudi::sEndpointContext(
			__kendp,
			metaDesc->name, metaDesc->initInfo,
			ops_idx, 0, channel_context);
	}
	else
	{
		newEndp = new lzudi::sEndpointContext(
			__kendp,
			NULL, NULL,
			ops_idx, 0, channel_context);
	}

	if (newEndp == NULL) {
		callback(gcb, NULL); return;
	};

	err = r->endpoints.insert(newEndp);
	if (err != ERROR_SUCCESS)
	{
		delete newEndp;
		callback(gcb, NULL); return;
	};

	callback(gcb, newEndp);
}

static lzudi::sRegion *getRegionMetadataForEndpointContext(
	lzudi::sEndpointContext *ec
	)
{
	List<lzudi::sRegion>		*metadataList;
	List<lzudi::sRegion>::Iterator	mlIt;
	Thread				*thread;

	thread = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();
	metadataList = &thread->getRegion()->parent->regionLocalMetadata;

	mlIt = metadataList->begin();
	for (; mlIt != metadataList->end(); ++mlIt)
	{
		lzudi::sRegion			*curr=*mlIt;

		if (curr->findEndpointContext(ec)) {
			return curr;
		};
	};

	return NULL;
}

void udi_channel_set_context(
	udi_channel_t target_channel,
	void *channel_context
	)
{
	lzudi::sRegion			*r=NULL;
	lzudi::sEndpointContext		*endpoint;

	endpoint = (lzudi::sEndpointContext *)target_channel;

	/* Just find the region-local metadata object on which the endpoint
	 * context is stored, then change its channel_context pointer. Scan all
	 * region-local metadata objects for one which contains the endpoint
	 * we were passed.
	 **/

	r = getRegionMetadataForEndpointContext(endpoint);
	// If r == NULL, the target_channel was invalid.
	if (r == NULL) { return; }

	endpoint->channel_context = channel_context;
}

void udi_channel_anchor(
	udi_channel_anchor_call_t *callback,
	udi_cb_t *gcb,
	udi_channel_t channel,
	udi_index_t ops_idx,
	void *channel_context
	)
{
	lzudi::sEndpointContext					*endp;
	lzudi::sRegion						*r;
	Thread							*thread;
	__klzbzcore::driver::CachedInfo				*drvInfoCache;
	__klzbzcore::driver::CachedInfo::sMetaDescriptor	*metaDesc;
	const udi_ops_init_t					*opsInit;

	/**	EXPLANATION:
	 * We need to set both the opsVector pointer in kernel space,
	 * and also the local bits of metadata that are derived from the
	 * ops_idx, and finally the channel_context pointer locally.
	 *
	 **	SEQUENCE:
	 * First thing is to determine whether or not the channel pointer is
	 * a valid endpoint. Following that, we can immediately just set the
	 * channel_context. Next, we can immediately proceed to gather the
	 * required extra metadata for the sEndpointContext (metaName, etc).
	 *
	 * Finally, we do the syscall and set the kernel-level opsVector
	 * pointer.
	 **/
	/* According to the spec, ops_idx must be non-zero for this call.
	 *
	 * Any failures will return a NULL argument because we would prefer to
	 * fail loudly than silently.
	 **/
	if (ops_idx == 0) {
		callback(gcb, NULL); return;
	};

	endp = (lzudi::sEndpointContext *)channel;
	thread = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();
	drvInfoCache = thread->parent->getDriverInstance()->getCachedInfo();

	fplainn::DriverInit		drvInfoParser(drvInfoCache->initInfo);

	/* By extension, if ops_idx cannot be 0, ops_idx must also resolve to a
	 * valid udi_ops_init_t entry.
	 **/
	opsInit = drvInfoParser.getOpsInit(ops_idx);
	if (opsInit == NULL) {
		callback(gcb, NULL); return;
	};

	r = getRegionMetadataForEndpointContext(endp);
	if (r == NULL) {
		callback(gcb, NULL); return;
	};

	endp->channel_context = channel_context;

	// Gather all information to be derived from ops_idx here.
	metaDesc = drvInfoCache->getMetaDescriptor(opsInit->meta_idx);
	if (metaDesc == NULL) {
		callback(gcb, NULL); return;
	};

	endp->anchor(metaDesc->name, metaDesc->initInfo, ops_idx);

	floodplainn.zudi.anchorEndpoint(
		endp->__kendpoint, opsInit->ops_vector, endp);

	callback(gcb, endp);
}

void udi_channel_close(udi_channel_t channel)
{
	(void)channel;
	UNIMPLEMENTED("udi_channel_close in libzudi");
	panic(ERROR_UNIMPLEMENTED);
}

static udi_layout_t blankInlineLayout[] = { UDI_DL_END };
void udi_mei_call(
	udi_cb_t *gcb,
	udi_mei_init_t *meta_info,
	udi_index_t meta_ops_num,
	udi_index_t vec_idx,
	...
	)
{
	enum layoutE {
		LAYOUT_VISIBLE, LAYOUT_MARSHAL, LAYOUT_INLINE };

	va_list				args;
	udi_layout_t			*layouts[3];
	lzudi::sEndpointContext		*endp;
	lzudi::sRegion			*r;
	Thread				*thread;
	lzudi::sControlBlock		*cbHeader;
	udi_mei_op_template_t		*opTemplate;

	/**	EXPLANATION:
	 * This is called by every metalanguage library. It is responsible for
	 * gathering the information required to marshal the call, and then
	 * calling the kernel to do the marshalling.
	 *
	 **	MARSHALING STRATEGY:
	 * The marshaling strategy is nothing novel; we simply follow the UDI
	 * specification's advice. However, the bit of marshaling that occurs
	 * /after/ that is what is tricky.
	 *
	 * We would adamantly prefer /not/ to introduce any new kind of IPC
	 * mechanism similar to ZAsyncStream. But the channel-IPC would require
	 * such a new mechanism, or else a method of completely embedding the
	 * IPC data into the kernel's own MessageStream::sHeader message which
	 * will be sent to the target region-thread.
	 *
	 * We will simply make use of the "MessageStream::sHeader::size" member
	 * to stuff the entire marshaled UDI message into our messagestream
	 * headers.
	 *
	 * At least for now, this is adequate. Ideally later on we should use
	 * some kind of robust alternative.
	 *
	 **	UDI Core Specification, Vol2, 28.3: Marshalling:
	 *		Each additional parameter after the control block
	 *		pointer, for a given channel operation, in left-to-right
	 *		order, shall be marshalled into the marshalling space
	 *		starting at offset zero and proceeding with successive
	 *		offsets.
	 *
	 **	In summary:
	 * * Take the arguments.
	 * * Get the "meta_info" argument.
	 * * Use the meta_info->...->visible_layout member to get the control
	 *   block layout.
	 * * Use the meta_info->...->marshal_layout member to get the argument
	 *   layout on the stack.
	 * * Analyze the control block's header-metadata to determine whether
	 *   or not it has a UDI_DL_INLINE_DRIVER_TYPED object within it. If
	 *   it does, get the udi_cb_init_t::inline_layout for the CB.
	 * * Pass all of these to send(), and let send() marshal the entire call
	 *   into a message.
	 **/
	if (meta_ops_num == 0) { return; };
	if (gcb == NULL || gcb->channel == NULL || meta_info == NULL)
	{
		// Can probably use that critical condition call API.
		return;
	};

	va_start(args, vec_idx);
	cbHeader = (lzudi::sControlBlock *)gcb;
	cbHeader--;
	endp = (lzudi::sEndpointContext *)gcb->channel;
	thread = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	// Now search for the endpoint among those anchored to this region.
	r = getRegionMetadataByIndex(thread->getRegion()->index);
	if (r == NULL) { return; };

	// If channel doesn't exist or is not bound to this region, abort.
	if (!r->findEndpointContext(endp))
	{
		printf(ERROR"udi_mei_call: endpContext %p passed by region "
			"%d doesn't exist.\n",
			endp, r->index);

		return;
	};

	fplainn::MetaInit			metaInfoParser(meta_info);

	opTemplate = metaInfoParser.getOpTemplate(meta_ops_num, vec_idx);
	if (opTemplate == NULL) { return; };

	layouts[LAYOUT_VISIBLE] = opTemplate->visible_layout;
	layouts[LAYOUT_MARSHAL] = opTemplate->marshal_layout;
	layouts[LAYOUT_INLINE] = cbHeader->driverTypedLayout;

	// Finally, pass the marshalling specification to Zudi::send().
	floodplainn.zudi.send(
		endp->__kendpoint,
		gcb, args, layouts,
		endp->metaName, meta_ops_num, vec_idx,
		gcb->origin);

	/**	EXPLanaTION:
	 * vec_idx==2 is enumerate_ack().
	 *
	 * If udi_primary_init_t::enumeration_attr_list_length is non-zero, we
	 * have to udi_mem_free() the mem we allocated for the attr_list before we
	 * udi_cb_free_sync() the cb.
	 **/
	if (!strncmp8(endp->metaName, CC"udi_mgmt", DRIVER_METALANGUAGE_MAXLEN)
		&& vec_idx == 2)
	{
		__klzbzcore::driver::CachedInfo		*drvInfoCache;

		// Possibly use udi_mei_driver_error() here.
		if (thread->parent->getType() != ProcessStream::DRIVER) { return; };
		drvInfoCache = thread->getRegion()->parent->device
			->driverInstance->getCachedInfo();

		if (drvInfoCache->initInfo->primary_init_info
			->enumeration_attr_list_length > 0)
		{
			udi_enumerate_cb_t		*ecb =
				(udi_enumerate_cb_t *)gcb;

			lzudi::udi_mem_free_sync(ecb->attr_list, 1);
		};
	};

	lzudi::udi_cb_free_sync(gcb);

}

static udi_layout_t		channel_event_cb[] =
{
	UDI_DL_UBIT8_T,
	  /* Union. We just instruct env to
	   * copy max bytes.
	   **/
	  UDI_DL_INLINE_UNTYPED,
	  UDI_DL_UBIT8_T,
	  UDI_DL_INLINE_UNTYPED,
	UDI_DL_END
};

static udi_layout_t		channel_event_complete_marshal_layout[] =
	{ UDI_DL_STATUS_T, UDI_DL_END };

static udi_layout_t		channel_event_complete_inline_layout[] =
	{ UDI_DL_END };

void _udi_channel_event_complete(udi_channel_event_cb_t *cb, ...)
{
	va_list			args;
	fplainn::Endpoint	*__kmgmtEndpoint;
	error_t			err;

	__kmgmtEndpoint = floodplainn.zudi.
		getMgmtEndpointForCurrentDeviceInstance();

	if (__kmgmtEndpoint == NULL)
	{
		printf(FATAL LZUDI"udi_chan_event_complete: failed to get "
			"device's end of the MGMT channel.\n"
			"\tUnable to send chan_event_complete response to "
			"ZUM MA.\n");

		return;
	};

	va_start(args, cb);

	udi_layout_t		*layouts[] =
	{
		channel_event_cb,
		channel_event_complete_marshal_layout,
		channel_event_complete_inline_layout
	};

	err = floodplainn.zudi.send(
		__kmgmtEndpoint,
		&cb->gcb, args, layouts,
		CC"udi_mgmt", UDI_MGMT_MA_OPS_NUM, 0,
		cb->gcb.origin);

	if (err != ERROR_SUCCESS) { return; };
}

void udi_channel_event_complete(udi_channel_event_cb_t *cb, udi_status_t status)
{
	_udi_channel_event_complete(cb, status);
}

void udi_mem_alloc (
	udi_mem_alloc_call_t *callback, udi_cb_t *gcb,
	udi_size_t size, udi_ubit8_t flags
	)
{
	void		*mem;

	mem = lzudi::udi_mem_alloc_sync(size, flags);
	if (mem == NULL) { callback(gcb, NULL); };

	callback(gcb, mem);
}

void *lzudi::udi_mem_alloc_sync(udi_size_t size, udi_ubit8_t flags)
{
	fplainn::sMovableMemory		*mem;

	if (size == 0) { return NULL; };

	// In the kernel we only give out movable memory.
	mem = new (size) fplainn::sMovableMemory(size);
	if (mem == NULL) { return NULL; };
	mem->setMagic(UDI_MOVABLE_MEM_MAGIC);

	mem++;
	if (!FLAG_TEST(flags, UDI_MEM_NOZERO)) {
		memset(mem, 0, size);
	};

	return mem;
}

void lzudi::udi_mem_free_sync(void *mem, sbit8 dontPanicOnBadMagic)
{
	fplainn::sMovableMemory		*memHdr;

	memHdr = (fplainn::sMovableMemory *)mem;
	memHdr--;
	if (!memHdr->magicIsValid())
	{
		printf(FATAL"udi_mem_free_sync: Found invalid magic!\n");
		if (!dontPanicOnBadMagic) {
			panic(ERROR_INVALID_STATE);
		}
		else
		{
			printf(FATAL"udi_mem_free_sync: dontPanicOnBadMagic "
				"flag set.\n");
		};

		return;
	};

	delete memHdr;
}

void udi_mem_free(void *mem)
{
	lzudi::udi_mem_free_sync(mem);
}

void *lzudi::sControlBlock::operator new(size_t sz, uarch_t payloadSize)
{
	return ::operator new(sz + payloadSize);
}

void lzudi::sControlBlock::operator delete(void *mem)
{
	::operator delete(mem);
}

void udi_cb_alloc(
	udi_cb_alloc_call_t *callback,
	udi_cb_t *gcb,
	udi_index_t cb_idx,
	udi_channel_t default_channel
	)
{
	error_t					err;
	udi_cb_t				*cb;
	__klzbzcore::driver::CachedInfo		*drvInfoCache;
	Thread					*caller;

	if (gcb == NULL) { callback(gcb, NULL); return; };

	caller = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();
	if (caller->parent->getType() != ProcessStream::DRIVER)
		{ callback(gcb, NULL); return; };

	drvInfoCache = caller->getRegion()->parent->device->driverInstance
		->getCachedInfo();

	err = lzudi::udi_cb_alloc_sync(drvInfoCache, cb_idx, &cb);
	if (err != ERROR_SUCCESS) { callback(gcb, NULL); };

	cb->context = gcb->context;
	cb->origin = gcb->origin;
	cb->channel = default_channel;
	callback(gcb, cb);
}

error_t lzudi::udi_cb_alloc_sync(
	__klzbzcore::driver::CachedInfo *drvInfoCache,
	udi_index_t cb_idx, udi_cb_t **retcb
	)
{
	udi_cb_init_t				*cbInit;
	__klzbzcore::driver::sMetaCbInfo	*metaCbDesc;
	fplainn::DriverInit
					drvInfoParser(drvInfoCache->initInfo);
	status_t				totalInlineSize;
	lzudi::sControlBlock			*cbTmp;
	uarch_t					dtypedLayoutNElems;

	cbInit = drvInfoParser.getCbInit(cb_idx);
	if (cbInit == NULL){ return ERROR_NOT_FOUND; };

	metaCbDesc = drvInfoCache->getMetaCbInfo(
		cbInit->meta_idx, cbInit->meta_cb_num);

	if (metaCbDesc == NULL) { return ERROR_NOT_FOUND; };

	/* Now using the visible_layout from the metaCbDesc that we cached
	 * in driverPath::main, we can analyze the layout of the visible
	 * portion of to-be-allocated CB, and determine how much inline memory
	 * will be required for it.
	 **/
	totalInlineSize = fplainn::sChannelMsg::getTotalCbInlineRequirements(
		metaCbDesc->visibleLayout, cbInit->inline_layout);

	if (totalInlineSize < 0) { return (error_t)totalInlineSize; };

	/* We also need to know how many elements are in the DRIVER_TYPED
	 * layout, if it's not NULL.
	 **/
	dtypedLayoutNElems = fplainn::sChannelMsg::getLayoutNElements(
		cbInit->inline_layout, 0);

	/* Next, we can now allocate the cb since we know how large it must
	 * be now. We allocate scratch separately to enable us to treat it
	 * as its own logical problem. Also, while we don't really care much,
	 * it does bring the benefit of shielding the driver from trampling the
	 * controlblock (which it could, if we'd allocated scratch as part of
	 * the CB itself, in one allocation).
	 *
	 * We could have used the udi_cb_init_t::scratch_requirement member,
	 * but since we have already cached the maximum size of the required
	 * scratch space for all CBs of a particular meta_idx+meta_cb_num,
	 * we might as well use that computed maximum value instead.
	 *
	 * Additionally, we copy the DRIVER_TYPED layout into the CB's hidden
	 * portion.
	 **/
	cbTmp = new (
		sizeof(udi_cb_t) + metaCbDesc->visibleSize
			+ totalInlineSize
			+ dtypedLayoutNElems)
		lzudi::sControlBlock;

	if (cbTmp == NULL) { return ERROR_MEMORY_NOMEM; };
	*retcb = (udi_cb_t *)++cbTmp;

	/* Next, we initialize the scratch pointer, and we have to
	 * initialize the pointers in the visible portion of
	 * the CB to point to their relevant inline space offsets.
	 **/
	if (metaCbDesc->scratchRequirement > 0)
	{
		(*retcb)->scratch = new ubit8[metaCbDesc->scratchRequirement];
		if ((*retcb)->scratch == NULL)
		{
			delete *retcb;
			return ERROR_MEMORY_NOMEM;
		};
	}
	else {
		(*retcb)->scratch = NULL;
	};

	// Finally, we copy the DRIVER_TYPED layout at the end of the CB.
	if (cbInit->inline_layout != NULL && dtypedLayoutNElems > 0)
	{
		udi_layout_t			*layoutOffset;
		lzudi::sControlBlock		*cbHdrTmp;

		cbHdrTmp = (lzudi::sControlBlock *)(*retcb - 1);
		layoutOffset = ((ubit8 *)*retcb) + sizeof(udi_cb_t)
				+ metaCbDesc->visibleSize
				+ totalInlineSize;

		cbHdrTmp->dtypedLayoutNElements = dtypedLayoutNElems;
		cbHdrTmp->driverTypedLayout = layoutOffset;

		memcpy(
			const_cast<ubit8 *>(layoutOffset),
			cbInit->inline_layout,
			dtypedLayoutNElems);
	};

	return fplainn::sChannelMsg::initializeCbInlinePointers(
		*retcb,
		((ubit8 *)*retcb) + sizeof(udi_cb_t) + metaCbDesc->visibleSize,
		metaCbDesc->visibleLayout, cbInit->inline_layout);
}

void udi_cb_free(udi_cb_t *cb)
{
	lzudi::udi_cb_free_sync(cb);
}

void lzudi::udi_cb_free_sync(udi_cb_t *cb)
{
	lzudi::sControlBlock		*cbHdr;

	cbHdr = (lzudi::sControlBlock *)cb;
	cbHdr--;

	if (!cbHdr->magicIsValid())
	{
		printf(FATAL"udi_cb_free_sync %p: Found invalid "
			"magic!\n",
			cbHdr);

		panic(ERROR_INVALID_STATE);
		return;
	}
	delete cbHdr;
}

lzudi::sControlBlock *lzudi::calleeCloneCb(
	fplainn::sChannelMsg *msg,
	udi_mei_op_template_t *opTemplate,
	udi_index_t opsIndex
	)
{
	sControlBlock			*ret;
	status_t			visibleSize, marshalSize,
					inlineObjectsSize;
	ubit8				*mcb8 = (ubit8 *)msg->getPayloadAddr(),
					*ret8;

	/**	EXPLANATION:
	 * Given a sChannelMsg, clone the CB inside of it based on the
	 * metaName+opsIndex information. The DRIVER_TYPED layout will be
	 * available as part of the message.
	 *
	 * In order, just find out the sizes of the sections of the CB
	 * (gcb, visible, inline-objects), and copy them over.
	 **/
	if (opsIndex == 0)
	{
		visibleSize = fplainn::sChannelMsg::zudi_layout_get_size(
			__klzbzcore::region::channel
				::mgmt::layouts::channel_event_cb, 1);

		marshalSize = fplainn::sChannelMsg::zudi_layout_get_size(
			__klzbzcore::region::channel
				::mgmt::layouts::channel_event_ind, 0);

		inlineObjectsSize = fplainn::sChannelMsg
			::getTotalMarshalSpaceInlineRequirements(
			__klzbzcore::region::channel
				::mgmt::layouts::channel_event_cb,
			msg->getDtypedLayoutAddr(),
			msg->getPayloadAddr());
	}
	else
	{
		visibleSize = fplainn::sChannelMsg::zudi_layout_get_size(
			opTemplate->visible_layout, 1);

		marshalSize = fplainn::sChannelMsg::zudi_layout_get_size(
			opTemplate->marshal_layout, 0);

		inlineObjectsSize = fplainn::sChannelMsg
			::getTotalMarshalSpaceInlineRequirements(
			opTemplate->visible_layout,
			msg->getDtypedLayoutAddr(),
			msg->getPayloadAddr());
	};

	if (visibleSize < 0 || marshalSize < 0 || inlineObjectsSize < 0)
	{
		printf(ERROR LZUDI"calleeCloneCb: unable to gauge size of one "
			"or more marshalled components in the sChannelMsg.\n"
			"\tRets: vis %d, marshal %d, inline %d.\n",
			visibleSize, marshalSize, inlineObjectsSize);

		panic(ERROR_NON_CONFORMANT);
		return NULL;
	};

	ret = new (
		sizeof(udi_cb_t)
			+ visibleSize + inlineObjectsSize
			+ (msg->dtypedLayoutNElements * sizeof(udi_layout_t)))
		sControlBlock;

	if (ret == NULL) { return NULL; };

	ret8 = (ubit8 *)(ret + 1);

	// Copy the GCB:
	memcpy(ret8, msg->getPayloadAddr(), sizeof(udi_cb_t));
	// Copy the visible portion.
	memcpy(
		ret8 + sizeof(udi_cb_t),
		mcb8 + sizeof(udi_cb_t),
		visibleSize);

	// Copy the inline objects over.
	memcpy(
		ret8 + sizeof(udi_cb_t) + visibleSize,
		mcb8 + sizeof(udi_cb_t) + visibleSize + marshalSize,
		inlineObjectsSize);

	// Copy the DRIVER_TYPED layout finally.
	if (msg->msgDtypedLayoutOff != 0)
	{
		ret->dtypedLayoutNElements = msg->dtypedLayoutNElements;
		ret->driverTypedLayout = (udi_layout_t *)(ret8 + sizeof(udi_cb_t)
			+ visibleSize
			+ inlineObjectsSize);

		memcpy(
			ret8 + sizeof(udi_cb_t)
				+ visibleSize + inlineObjectsSize,
			msg->getDtypedLayoutAddr(),
			msg->dtypedLayoutNElements);
	};

	return ret;
}
