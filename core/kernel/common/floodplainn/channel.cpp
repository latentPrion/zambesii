
#include <kernel/common/floodplainn/channel.h>


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
