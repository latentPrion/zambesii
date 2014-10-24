
#include <__kstdlib/__kclib/string8.h>
#include <kernel/common/thread.h>
#include <kernel/common/process.h>
#include <kernel/common/floodplainn/channel.h>
#include <kernel/common/floodplainn/zudi.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


void fplainn::Channel::operator delete(void *obj)
{
	Channel		*chan = reinterpret_cast<Channel *>(obj);

	/**	EXPLANATION:
	 * Channels always begin as an IncompleteChannel derivative, and then
	 * they are cast downwards to a Channel derivative.
	 *
	 * We must ensure that we don't delete() the Channel derivative pointer,
	 * and that we cast the channel back up to its IncompleteChannel
	 * derivative before deleting.
	 **/
	if (chan->getType() == TYPE_D2D) {
		::delete static_cast<IncompleteD2DChannel *>(chan);
	} else {
		::delete static_cast<IncompleteD2SChannel *>(chan);
	};
}

fplainn::D2DChannel::D2DChannel(IncompleteD2DChannel &ic)
: Channel(&regionEnd1),
regionEnd1(this, &regionEnd0)
{
	endpoints[1] = &regionEnd1;

	endpoints[0]->anchor(
		ic.regionEnd0.region,
		ic.endpoints[0]->opsVector, ic.endpoints[0]->privateData);

	endpoints[1]->anchor(
		ic.regionEnd1.region,
		ic.endpoints[1]->opsVector, ic.endpoints[1]->privateData);
}

fplainn::D2SChannel::D2SChannel(IncompleteD2SChannel &ic)
: Channel(&fstreamEnd),
fstreamEnd(this, &regionEnd0)
{
	endpoints[1] = &fstreamEnd;

	endpoints[0]->anchor(
		ic.regionEnd0.region, ic.endpoints[0]->opsVector,
		ic.endpoints[0]->privateData);

	endpoints[1]->anchor(
		ic.fstreamEnd.thread, ic.endpoints[1]->opsVector,
		ic.endpoints[1]->privateData);
}

void *fplainn::sChannelMsg::operator new(size_t sz, uarch_t dataSize)
{
	return ::operator new(sz + dataSize);
}

error_t fplainn::sChannelMsg::send(
	fplainn::Endpoint *endp,
	udi_cb_t *gcb, va_list args, udi_layout_t *layouts[3],
	utf8Char *metaName, udi_index_t meta_ops_num, udi_index_t ops_idx,
	void *privateData
	)
{
	enum layoutE {
		LAYOUT_VISIBLE, LAYOUT_MARSHAL, LAYOUT_INLINE };

	fplainn::sChannelMsg			*msg;
//	Thread					*thread;
	uarch_t					visibleSize, marshalSize,
						inlineSize,
						totalInlineSize,
						totalMovableSize;
	udi_ubit16_t				inlineOffset, dummy;
	ubit8					*gcb8 = (ubit8 *)gcb,
						*data8;

	(void)totalInlineSize; (void)totalMovableSize;

	/**	EXPLANATION:
	 * In this one we need to do a little more work; we must get the
	 * region for the current thread (only driver processes should call this
	 * function), and then we can proceed.
	 **/
	if (gcb == NULL || metaName == NULL || meta_ops_num == 0)
		{ return ERROR_INVALID_ARG; };

	if (layouts == NULL
		|| layouts[LAYOUT_VISIBLE] == NULL
		|| layouts[LAYOUT_MARSHAL] == NULL)
		{ return ERROR_NON_CONFORMANT; };

//	thread = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	visibleSize = fplainn::sChannelMsg::_udi_get_layout_size(
		layouts[LAYOUT_VISIBLE], &inlineOffset, &dummy);
//	visibleSize = fplainn::sChannelMsg::zudi_get_layout_size(
//		layouts[LAYOUT_VISIBLE]);

	marshalSize = fplainn::sChannelMsg::_udi_get_layout_size(
		layouts[LAYOUT_MARSHAL], &dummy, &dummy);

	inlineSize = fplainn::sChannelMsg::_udi_get_layout_size(
		layouts[LAYOUT_INLINE], &dummy, &dummy);

printf(NOTICE"Vis %d, marshal %d, inline %d.\n",
	visibleSize, marshalSize, inlineSize);

	msg = new (sizeof(udi_cb_t) + visibleSize + marshalSize + inlineSize)
		fplainn::sChannelMsg(
			0,
			MSGSTREAM_SUBSYSTEM_ZUDI, MSGSTREAM_ZUDI_CHANNEL_SEND,
			sizeof(*msg), 0, privateData);

	if (msg == NULL) { return ERROR_MEMORY_NOMEM; };
	data8 = (ubit8 *)msg->data;

	/* At this point, we have enough memory to hold almost everything.
	 * This marshalling sequence does not support inline elements properly.
	 *
	 * Specifically, it supports UDI_DL_INLINE_DRIVER_TYPED inline objects,
	 * but no others.
	 *
	 * This can be easily remedied.
	 **/
	// Copy the generic control block.
	memcpy(data8, gcb8, sizeof(udi_cb_t));
	// Copy the visible portion of the meta-specific control block area.
	memcpy(
		data8 + sizeof(udi_cb_t), gcb8 + sizeof(udi_cb_t),
		visibleSize);

	// Copy the stack arguments.
	memcpy(
		data8 + sizeof(udi_cb_t) + visibleSize,
		args, marshalSize);

	/* All messages without inline pointers will now be fully marshalled.
	 * This next portion is for marshalling inline pointers. We scan through
	 * the entirety of the visible_layout seeking inline pointers.
	 *
	 * For each pointer, we:
	 *	1. Get the offset within the sourceCb where it is,
	 *	   and use that to extract the address of the inline object.
	 *	2. Use the visible_layout to determine how large the inline
	 *	   object is.
	 *	3. Copy the inline object into our sChannelMsg's inline
	 *	   marshalling space.
	 *	4. Set the sChannelMsg's inline pointer to point to the offset
	 *	   within itself that now holds the copied inline object.
	 *
	 * 	* Special case: UDI_DL_INLINE_DRIVER_TYPED:
	 *		The layout of a DRIVER_TYPED inline object is passed to
	 *		us as an argument. We simply use its size and offset
	 *		to copy it in, along with the other inline objects.
	 *
	 * FOR NOW however, we only support UDI_DL_INLINE_DRIVER_TYPED.
	 **/
	if (layouts[LAYOUT_INLINE] != NULL)
	{
		memcpy(
			data8 + sizeof(udi_cb_t) + visibleSize + marshalSize,
			*(void **)(gcb8 + inlineOffset), inlineSize);

		*(void **)(data8 + inlineOffset) = data8 + sizeof(udi_cb_t)
			+ visibleSize + marshalSize;
	};

	msg->set(metaName, meta_ops_num, ops_idx);
	msg->setPayloadSize(
		sizeof(udi_cb_t) + visibleSize + marshalSize + inlineSize);

	return endp->send(msg);
}

void fplainn::Region::dumpChannelEndpoints(void)
{
	PtrList<RegionEndpoint>::Iterator	it;

	for (it=endpoints.begin(0); it != endpoints.end(); ++it)
	{
		RegionEndpoint			*rendp = *it;

		rendp->dump();
	};
}

fplainn::IncompleteChannel *
fplainn::Channel::getIncompleteChannelBySpawnIndex(
	udi_index_t spawn_idx
	)
{
	PtrList<IncompleteChannel>::Iterator	it;

	for (it = incompleteChannels.begin(0); it != incompleteChannels.end();
		++it)
	{
		IncompleteChannel		*incChan = *it;

		if (incChan->spawnIndex == spawn_idx) { return incChan; };
	};

	return NULL;
}

error_t fplainn::Channel::createIncompleteChannel(
	typeE type, udi_index_t spawnIdx, IncompleteChannel **retIncChan
	)
{
	IncompleteChannel		*tmp;
	error_t				ret;

	if (retIncChan == NULL) { return ERROR_INVALID_ARG; };

	tmp = getIncompleteChannelBySpawnIndex(spawnIdx);
	if (tmp != NULL) { return ERROR_DUPLICATE; };

	if (type == TYPE_D2D)
	{
		tmp = static_cast<IncompleteChannel *>(
			new IncompleteD2DChannel(spawnIdx));

		if (tmp == NULL) { return ERROR_MEMORY_NOMEM; };
		ret = static_cast<IncompleteD2DChannel *>(tmp)->initialize();
	}
	else
	{
		tmp = static_cast<IncompleteChannel *>(
			new IncompleteD2SChannel(spawnIdx));

		if (tmp == NULL) { return ERROR_MEMORY_NOMEM; };
		ret = static_cast<IncompleteD2SChannel *>(tmp)->initialize();
	};

	if (ret != ERROR_SUCCESS) { return ret; };

	incompleteChannels.insert(tmp);
	*retIncChan = tmp;
	return ERROR_SUCCESS;
}

sbit8 fplainn::Channel::removeIncompleteChannel(
	typeE, udi_index_t spawnIdx
	)
{
	sbit8				ret;
	IncompleteChannel		*tmp;

	tmp = getIncompleteChannelBySpawnIndex(spawnIdx);
	if (tmp == NULL) { return 0; };

	ret = incompleteChannels.remove(tmp);
	return ret;
}

error_t fplainn::FStreamEndpoint::anchor(void)
{
	if (!isAnchored()) { return ERROR_UNINITIALIZED; }
	return thread->parent->floodplainnStream.addEndpoint(this);
}

error_t fplainn::RegionEndpoint::anchor(void)
{
	if (!isAnchored()) { return ERROR_UNINITIALIZED; }
	return region->addEndpoint(this);
}

error_t fplainn::Endpoint::enqueue(sChannelMsg *msg)
{
	msg->endpointPrivateData = privateData;
	msg->opsVector = opsVector;
	msg->__kendpoint = this;
	return MessageStream::enqueueOnThread(
		msg->header.targetId, &msg->header);
}

error_t fplainn::RegionEndpoint::enqueue(sChannelMsg *msg)
{
	/**	EXPLANATION:
	 * At this point, UDI call data has been marshalled into one contiguous
	 * block. This UDI call message was then passed to
	 * FloodplainnStream::send() or Zudi::send().
	 *
	 * Those send() functions took the UDI call message and embedded it into
	 * an fplainn::sChannelMsg. It then gave that over to the endpoint
	 * opposite us via Endpoint::send(). That endpoint simply passed it over
	 * to us to be enqueued.
	 *
	 * Now we must enqueue the message on the thing that we have been
	 * anchored to. For us specifically, that is a region since we are
	 * the RegionEndpoint class.
	 **/
	if (!isAnchored()) { return ERROR_UNINITIALIZED; };

	msg->header.targetId = region->thread->getFullId();
	return Endpoint::enqueue(msg);
}

error_t fplainn::FStreamEndpoint::enqueue(sChannelMsg *msg)
{
	/**	EXPLANATION:
	 * Same as RegionEndpoint::enqueue(), except that we enqueue on our
	 * back-end ::Thread; not on a region.
	 **/
	if (!isAnchored()) { return ERROR_UNINITIALIZED; };

	msg->header.targetId = thread->getFullId();
	return Endpoint::enqueue(msg);
}

void fplainn::FStreamEndpoint::dump(void)
{
	Endpoint::dump();
	printf(CC"fpstream pid 0x%x, listenertid 0x%x.\n",
		thread->parent->id, thread->getFullId());
}

void fplainn::RegionEndpoint::dump(void)
{
	Endpoint::dump();
	printf(CC"dev %s, region idx %d, tid 0x%x.\n",
		region->parent->device->driverInstance->driver->shortName,
		region->index, region->thread->getFullId());
}
