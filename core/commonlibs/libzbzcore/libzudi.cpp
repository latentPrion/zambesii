
#define UDI_VERSION	0x101
#include <udi.h>
#include <commonlibs/libzbzcore/libzudi.h>
#include <kernel/common/thread.h>
#include <kernel/common/process.h>
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
	strncpy8(this->metaName, metaName, ZUI_DRIVER_METALANGUAGE_MAXLEN);
	this->metaName[ZUI_DRIVER_METALANGUAGE_MAXLEN - 1] = '\0';

	fplainn::MetaInit		metaInit(metaInfo);

	opsVectorTemplate = metaInit.getOpsVectorTemplate(opsIdx);
}

void lzudi::sEndpointContext::dump(void)
{
	printf(NOTICE"endpointCtxt: 0x%p: metaInfo 0x%p, metaName %s.\n"
		"\t__kendp 0x%p, chan_ctxt 0x%p, opsVecTemplate 0x%p.\n",
		this, metaInfo, metaName, __kendpoint, channel_context,
		opsVectorTemplate);
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

