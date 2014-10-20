
#define UDI_VERSION	0x101
#include <udi.h>
#include <commonlibs/libzbzcore/libzudi.h>
#include <commonlibs/libzbzcore/libzbzcore.h>
#include <kernel/common/thread.h>
#include <kernel/common/process.h>
#include <kernel/common/panic.h>
#include <kernel/common/floodplainn/floodplainn.h>
#include <kernel/common/floodplainn/initInfo.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


lzudi::sEndpointContext::sEndpointContext(
	fplainn::Endpoint *__kendp,
	utf8Char *metaName,
	udi_mei_init_t *metaInfo, udi_index_t opsIdx,
	void *channel_context
	)
:
__kendpoint(__kendp), metaInfo(metaInfo), channel_context(channel_context)
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
	printf(NOTICE"endpointCtxt: 0x%p: metaInfo 0x%p, metaName %s.\n"
		"\t__kendp 0x%p, chan_ctxt 0x%p, opsVecTemplate 0x%p.\n",
		this, metaInfo, metaName, __kendpoint, channel_context,
		opsVectorTemplate);
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
	List<lzudi::sRegion>		*metadataList;
	List<lzudi::sRegion>::Iterator	mlIt;
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
	metadataList = &thread->getRegion()->parent->regionLocalMetadata;
	drvInfoCache = thread->parent->getDriverInstance()->cachedInfo;

	// Really hate that we have to do this loop, but no way around it.
	mlIt = metadataList->begin();
	for (; mlIt != metadataList->end(); ++mlIt)
	{
		lzudi::sRegion		*tmp = *mlIt;

		if (tmp->index != thread->getRegion()->index) { continue; };
		// Found it.
		r = tmp;
		break;
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

	err = floodplainn.zudi.spawnEndpoint(
		originEndp->__kendpoint, spawn_idx,
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
			ops_idx, channel_context);
	}
	else
	{
		newEndp = new lzudi::sEndpointContext(
			__kendp,
			NULL, NULL,
			ops_idx, channel_context);
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

static lzudi::sRegion *getRegionForEndpointContext(lzudi::sEndpointContext *ec)
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

	r = getRegionForEndpointContext(endpoint);
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
	fplainn::DriverInstance					*drvInst;
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
	// According to the spec, ops_idx must be non-zero for this call.
	if (ops_idx == 0) { return; };

	endp = (lzudi::sEndpointContext *)channel;
	thread = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();
	drvInfoCache = thread->parent->getDriverInstance()->cachedInfo;

	fplainn::DriverInit		drvInfoParser(drvInfoCache->initInfo);

	/* By extension, if ops_idx cannot be 0, ops_idx must also resolve to a
	 * valid udi_ops_init_t entry.
	 **/
	opsInit = drvInfoParser.getOpsInit(ops_idx);
	if (opsInit == NULL) { return; };

	r = getRegionForEndpointContext(endp);
	if (r == NULL) { return; };

	endp->channel_context = channel_context;

	// Gather all information to be derived from ops_idx here.
	metaDesc = drvInfoCache->getMetaDescriptor(opsInit->meta_idx);
	endp->anchor(metaDesc->name, metaDesc->initInfo, ops_idx);

	floodplainn.zudi.anchorEndpoint(
		endp->__kendpoint, opsInit->ops_vector, endp);
}

void udi_channel_close(udi_channel_t channel)
{
}

void udi_mei_call(
	udi_cb_t *gcb,
	udi_mei_init_t *meta_info,
	udi_index_t meta_ops_num,
	udi_index_t vec_idx,
	...
	)
{
	Thread				*self;
	udi_ubit16_t			dummy;
	udi_layout_t			*visible, *marshal, *inlin;
	udi_size_t			visibleSize, inlineSize, marshalSize,
	// totalSize begins at size of a generic CB, then we add to it.
					totalSize = sizeof(udi_cb_t);
	udi_mei_ops_vec_template_t	*opsVector;
	udi_mei_op_template_t		*op;

	/**	EXPLANATION:
	 * This is called by every metalanguage library. It is responsible for
	 * marshaling the arguments for the call into a single block and then
	 * calling the kernel.
	 *
	 * In addition, it must consider the type of process on whose behalf it
	 * is preparing the call:
	 *	* For driver hosts, it must call floodplainn.zudi.send().
	 *		This syscall will interpret the "channel" argument of
	 *		the generic control block passed to it as one end
	 *		of a Driver-to-Driver channel.
	 *		Consequently, it will get the region for the current
	 *		thread, and scan it for all channels bound to it. If the
	 *		"channel" passed through the control block is not found,
	 *		it will assume that the "channel" pointer is invalid.
	 *	* For non-driver process hosts, it must call
	 *	  floodplainnStream.send().
	 *		This syscall will interpret the "channel" argument of
	 *		the generic control block passed to it as one end of
	 *		a Driver-to-Stream channel.
	 *		Consequently, when trying to validate the channel
	 *		pointer, it will scan only through the list of endpoints
	 *		on the host process' FloodplainnStream instance.
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
	 * * Combine these to deduce the total size of extra storage required
	 *   for the marshalled block.
	 * * Allocate a kernel message structure with extra room at the end
	 *   to hold the entire thing.
	 * * Marshal all arguments into the allocated space.
	 * * </end>.
	 **/
	if (gcb == NULL || meta_info == NULL) { return; };
	if (meta_ops_num == 0) { return; };

	fplainn::MetaInit				metaInfo(meta_info);

	self = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	opsVector = metaInfo.getOpsVectorTemplate(meta_ops_num);
	if (opsVector == NULL) { return; };
	op = metaInfo.getOpTemplate(opsVector, vec_idx);
	if (op == NULL) { return; };

	visibleSize = lzudi::_udi_get_layout_size(
		op->visible_layout, &dummy, &dummy);

	marshalSize = lzudi::_udi_get_layout_size(
		op->marshal_layout, &dummy, &dummy);

	// We don't need to consider anything to do with scratch here.
printf(NOTICE"visiblesize %d, marshalsize %d.\n", visibleSize, marshalSize);
	// Eventually.... We send all the marshalled data as one clump.
	if (self->parent->getType() == ProcessStream::DRIVER) {
//		floodplainn.zudi.send(MARSHALLED_DATA);
	} else {
//		self->parent->floodplainnStream.send(NULL);
	};
}

