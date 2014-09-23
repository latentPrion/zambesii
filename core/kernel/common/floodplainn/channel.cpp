
#include <kernel/common/floodplainn/channel.h>
#include <kernel/common/floodplainn/zudi.h>
#include <kernel/common/thread.h>
#include <kernel/common/process.h>


fplainn::D2DChannel::D2DChannel(IncompleteD2DChannel &ic)
: Channel(&regionEnd1),
regionEnd1(this, &regionEnd0)
{
	endpoints[1] = &regionEnd1;

	endpoints[0]->anchor(
		ic.regionEnd0.region,
		ic.endpoints[0]->opsVector, ic.endpoints[0]->channelContext);

	endpoints[1]->anchor(
		ic.regionEnd1.region,
		ic.endpoints[1]->opsVector, ic.endpoints[1]->channelContext);
}

fplainn::D2SChannel::D2SChannel(IncompleteD2SChannel &ic)
: Channel(&fstreamEnd),
fstreamEnd(this, &regionEnd0)
{
	endpoints[1] = &fstreamEnd;

	endpoints[0]->anchor(
		ic.regionEnd0.region, ic.endpoints[0]->opsVector,
		ic.endpoints[0]->channelContext);

	endpoints[1]->anchor(
		ic.fstreamEnd.thread, ic.endpoints[1]->opsVector,
		ic.endpoints[1]->channelContext);
}

void *fplainn::sChannelMsg::operator new(size_t sz, uarch_t dataSize)
{
	return ::operator new(sz + dataSize);
}

error_t fplainn::sChannelMsg::send(
	fplainn::Endpoint *endp, udi_cb_t *mcb, uarch_t size, void *privateData
	)
{
	fplainn::sChannelMsg		*msg;

	/**	EXPLANATION:
	 * The back-end for the other ZUDI_CHANNEL_SEND category APIs.
	 * Allocates an sChannelMsg, populates its members and prepares it, then
	 * calls Endpoint::send() to send it down the channel.
	 **/
	// Custom sChannelMsg::operator new().
	msg = new (size) sChannelMsg(
		/* We don't know the targetTid of the message. It will be filled
		 * out by the channel while it is passing through, because only
		 * the channel knows where its ends point.
		 **/
		0,
		MSGSTREAM_SUBSYSTEM_ZUDI, MSGSTREAM_ZUDI_CHANNEL_SEND,
		sizeof(*msg), 0, privateData);

	if (msg == NULL)
	{
		printf(ERROR FPSTREAM"send %dB: Failed to alloc sChannelMsg.\n",
			size);

		return ERROR_MEMORY_NOMEM;
	};

	msg->set(size);
	memcpy(msg->data, mcb, size);
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

	if (retIncChan == NULL) { return ERROR_INVALID_ARG; };

	tmp = getIncompleteChannelBySpawnIndex(spawnIdx);
	if (tmp != NULL) { return ERROR_DUPLICATE; };

	if (type == TYPE_D2D)
	{
		tmp = static_cast<IncompleteChannel *>(
			new IncompleteD2DChannel(spawnIdx));
	}
	else
	{
		tmp = static_cast<IncompleteChannel *>(
			new IncompleteD2SChannel(spawnIdx));
	};

	if (tmp == NULL) { return ERROR_MEMORY_NOMEM; };

	incompleteChannels.insert(tmp);
	*retIncChan = tmp;
	return ERROR_SUCCESS;
}

sbit8 fplainn::Channel::destroyIncompleteChannel(
	typeE type, udi_index_t spawnIdx
	)
{
	sbit8				ret;
	IncompleteChannel		*tmp;

	tmp = getIncompleteChannelBySpawnIndex(spawnIdx);
	if (tmp == NULL) { return 0; };

	ret = incompleteChannels.remove(tmp);

	if (type == TYPE_D2D) {
		delete static_cast<IncompleteD2DChannel *>(tmp);
	} else {
		delete static_cast<IncompleteD2SChannel *>(tmp);
	}

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
	return MessageStream::enqueueOnThread(
		msg->header.targetId, &msg->header);
}

error_t fplainn::FStreamEndpoint::enqueue(sChannelMsg *msg)
{
	/**	EXPLANATION:
	 * Same as RegionEndpoint::enqueue(), except that we enqueue on our
	 * back-end ::Thread; not on a region.
	 **/
	if (!isAnchored()) { return ERROR_UNINITIALIZED; };

	msg->header.targetId = thread->getFullId();
	return MessageStream::enqueueOnThread(
		msg->header.targetId, &msg->header);
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
