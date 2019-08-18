
#include <__kstdlib/__kcxxlib/memory>
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kclib/string.h>
#include <kernel/common/process.h>
#include <kernel/common/panic.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/floodplainn/floodplainn.h>
#include <kernel/common/floodplainn/zudi.h>
#include <kernel/common/floodplainn/device.h>
#include <kernel/common/floodplainn/movableMemory.h>


void *fplainn::sMovableMemory::operator new(size_t sz, uarch_t objectSize)
{
	return ::operator new(sz + objectSize);
}

void fplainn::sMovableMemory::operator delete(void *mem)
{
	::operator delete(mem);
}

error_t fplainn::Zudi::getInternalBopInfo(
	ubit16 regionIndex, ubit16 *metaIndex,
	ubit16 *opsIndex0, ubit16 *opsIndex1,
	ubit16 *bindCbIndex
	)
{
	Thread				*self;
	fplainn::DriverInstance		*drvInst;
	fplainn::Driver::sInternalBop	*iBop;

	self = (Thread *)cpuTrib.getCurrentCpuStream()
		->taskStream.getCurrentThread();

	drvInst = self->parent->getDriverInstance();

	iBop = drvInst->driver->getInternalBop(regionIndex);
	if (iBop == NULL) { return ERROR_NOT_FOUND; };

	*metaIndex = iBop->metaIndex;
	*opsIndex0 = iBop->opsIndex0;
	*opsIndex1 = iBop->opsIndex1;
	*bindCbIndex = iBop->bindCbIndex;
	return ERROR_SUCCESS;
}

fplainn::Channel::bindChannelTypeE fplainn::Zudi::getBindChannelType(
	fplainn::Endpoint *endp
	)
{
	Thread			*self;
	fplainn::Channel	*chan;

	if (endp == NULL) { return fplainn::Channel::BIND_CHANNEL_TYPE_NONE; };

	self = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	if (self->parent->getType() != ProcessStream::DRIVER)
		{ return fplainn::Channel::BIND_CHANNEL_TYPE_NONE; };

	chan = self->getRegion()->parent->getChannelByEndpoint(endp);
	if (chan == NULL) { return fplainn::Channel::BIND_CHANNEL_TYPE_NONE; };
	return chan->bindChannelType;
}

error_t fplainn::Zudi::getEndpointMetaName(
	fplainn::Endpoint *endp, utf8Char *mem
	)
{
	Thread			*self;
	fplainn::Channel	*chan;

	if (endp == NULL || mem == NULL) { return ERROR_INVALID_ARG; };

	self = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	if (self->parent->getType() != ProcessStream::DRIVER)
		{ return ERROR_UNAUTHORIZED; };

	chan = self->getRegion()->parent->getChannelByEndpoint(endp);
	if (chan == NULL) { return ERROR_NO_MATCH; };

	strncpy8(mem, chan->metaName, DRIVER_METALANGUAGE_MAXLEN);
	return ERROR_SUCCESS;
}

fplainn::Endpoint *fplainn::Zudi::getMgmtEndpointForCurrentDeviceInstance(void)
{
	Thread			*self;

	self = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();
	if (self->parent->getType() != ProcessStream::DRIVER) { return NULL; };

	if (self->getRegion()->parent->mgmtEndpoint == NULL) { return NULL; };
	return self->getRegion()->parent->mgmtEndpoint->otherEnd;
}

error_t fplainn::Zudi::spawnEndpoint(
	fplainn::Endpoint *channel_endp,
	utf8Char *metaName, udi_index_t spawn_idx,
	udi_ops_vector_t *ops_vector,
	udi_init_context_t *channel_context,
	fplainn::Endpoint **retendp
	)
{
	error_t					ret;
	Thread					*self;
	fplainn::Region				*callerRegion;
	fplainn::Channel			*originChannel;
	fplainn::IncompleteChannel		*incChan;
	sbit8					emptyEndpointIdx;

	(void)retendp;
	/**	EXPLANATION:
	 * This is a syscall. Only drivers are allowed to call this.
	 *
	 * We need to get the device instance for the caller, and:
	 *	* Check to see if a channel endpoint matching the pointer passed
	 * 	  to us exists.
	 * 	* Take the channel which is the parent of the passed endpoint,
	 *	  and check to see if it has an incompleteChannel with the
	 *	  spawn_idx passed to us here.
	 * 		* If an incompleteChannel with our spawn_idx value
	 *		  doesn't exist, create a new incompleteChannel and
	 *		  then anchor() it.
	 *		* If one exists, anchor the new end, then remove it
	 *		  from the incompleteChannel list, and create a new
	 *		  fully fledged channel based on it.
	 **/
	self = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	if (self->parent->getType() != ProcessStream::DRIVER)
		{ return ERROR_UNAUTHORIZED; };

	// Set by fplainn::DeviceInstance::setThreadRegionPointer().
	callerRegion = self->getRegion();
	// Make sure the "channel" endpoint passed to us is really an endpoint.
	originChannel = callerRegion->parent->getChannelByEndpoint(channel_endp);
	if (originChannel == NULL) { return ERROR_INVALID_ARG_VAL; };

	// Is there already a spawn in progress with this spawn_idx?
	incChan = originChannel->getIncompleteChannelBySpawnIndex(spawn_idx);
	if (incChan == NULL)
	{
		/* No incomplete channel with our spawn_idx exists. Create a
		 * new incomplete channel and anchor one of its endpoints.
		 *
		 * Endpoint0 is always a region endpoint. Since this is
		 **/
		ret = originChannel->createIncompleteChannel(
			originChannel->getType(),
			metaName, Channel::BIND_CHANNEL_TYPE_NONE,
			spawn_idx, &incChan);

		/* We can use originChannel to determine what type of incomplete
		 * channel (D2D or D2S) we have spawned because it is not
		 * possible for a D2D channel to be used to spawn anything but
		 * another D2D channel, and vice versa: a D2S channel can only
		 * be used to spawn another D2S channel.
		 **/
		incChan->cast2BaseChannel(originChannel->getType())
			->endpoints[0]->anchor(
				callerRegion, ops_vector, channel_context);

		incChan->setOrigin(0, channel_endp);
		*retendp = incChan->cast2BaseChannel(originChannel->getType())
			->endpoints[0];

		return ERROR_SUCCESS;
	};

	/* Check to ensure that the same caller doesn't call spawn() twice and
	 * set up a channel to itself like a retard. If spawn() is called twice
	 * with the same originating endpoint, we cause the second call to
	 * overwrite the attributes that were set by the first.
	 **/
	for (uarch_t i=0; i<2; i++)
	{
		if (incChan->originEndpoints[i] == channel_endp)
		{
			// If it's a repeat call, just re-anchor (overwrite).
			incChan->cast2BaseChannel(originChannel->getType())
				->endpoints[i]->anchor(
					callerRegion, ops_vector,
					channel_context);

			*retendp = incChan->cast2BaseChannel(
				originChannel->getType())->endpoints[i];

			return ERROR_SUCCESS;
		};

		// Prepare for below.
		if (incChan->originEndpoints[i] == NULL) {
			emptyEndpointIdx = i;
		};
	};

	// Else anchor the other end.
	incChan->cast2BaseChannel(originChannel->getType())
		->endpoints[emptyEndpointIdx]->anchor(
			callerRegion, ops_vector, channel_context);

	/* Now, if both ends have been claimed by calls to udi_channel_spawn()
	 * such that both ends have valid "parentEndpoint"s, we can proceed
	 * to create a new channel based on the incomplete channel.
	 **/
	if (incChan->originEndpoints[0] == NULL
		|| incChan->originEndpoints[1] == NULL)
	{
		panic(
			ERROR_FATAL,
			FATAL ZUDI"udi_ch_spawn: originating endpoints are "
			"NULL at point where that should not be possible.\n");
	};

	// If we're here, then both ends have been claimed and matched.
	if (originChannel->getType() == fplainn::Channel::TYPE_D2D)
	{
		fplainn::D2DChannel		*dummy;

		ret = createChannel(
			static_cast<IncompleteD2DChannel *>(incChan), &dummy);
	}
	else
	{
		fplainn::D2SChannel		*dummy;

		ret = FloodplainnStream::createChannel(
			static_cast<IncompleteD2SChannel *>(incChan), &dummy);
	};

	if (ret != ERROR_SUCCESS) { return ret; };

	// Next, remove the incompleteChannel from the originating channel.
	originChannel->removeIncompleteChannel(
		originChannel->getType(), incChan->spawnIndex);

	*retendp = incChan->cast2BaseChannel(originChannel->getType())
		->endpoints[emptyEndpointIdx];

	return ERROR_SUCCESS;
}

void fplainn::Zudi::anchorEndpoint(
	fplainn::Endpoint *endp, udi_ops_vector_t *ops_vector,
	void *endpPrivateData
	)
{
	Thread			*currThread;
	fplainn::Channel	*chanTmp;

	currThread = cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentThread();

	if (currThread->parent->getType() != ProcessStream::DRIVER)
		{ return; };

	chanTmp = currThread->getRegion()->parent->getChannelByEndpoint(endp);
	if (chanTmp == NULL) { return; };

	endp->anchor(
		static_cast<fplainn::RegionEndpoint *>(endp)->region,
		ops_vector, endpPrivateData);
}

void fplainn::Zudi::setEndpointPrivateData(
	fplainn::Endpoint *endp, void *privateData
	)
{
	Thread			*currThread;
	fplainn::Channel	*chanTmp;

	/**	NOTES:
	 * Since this is a kernel syscall, and it is not actually
	 * udi_channel_set_context(), we can impose the restriction that only
	 * the region to which a channel is bound can call this function to set
	 * its context.
	 *
	 * In any case, we would have to scan through incomplete channels as
	 * well if we want to support setting context from regions to which the
	 * endpoint is not bound.
	 *
	 **	EXPLANATION:
	 * Now from here, we get the region for the current (calling) thread,
	 * and then scan through its bound endpoints. If we find an endpoint
	 * which matches the pointer the caller passed to us, we proceed to
	 * change the privatedata for that endpoint.
	 **/

	currThread = cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentThread();

	if (currThread->parent->getType() != ProcessStream::DRIVER)
		{ return; };

	chanTmp = currThread->getRegion()->parent->getChannelByEndpoint(endp);
	if (chanTmp == NULL) { return; };

	endp->anchor(
		static_cast<fplainn::RegionEndpoint *>(endp)->region,
		endp->opsVector, privateData);
}

error_t fplainn::Zudi::createChannel(
	fplainn::IncompleteD2DChannel *bprint, fplainn::D2DChannel **retchan
	)
{
	fplainn::D2DChannel		*chan;
	error_t				ret0, ret1;
	fplainn::Region			*region0, *region1;

	/**	EXPLANATION:
	 * Back-end function that actually allocates the channel object and
	 * adds it to both device-instances' channel lists.
	 *
	 * The region thread, ops-vector and channel-context need not be
	 * set at this point, but they will be required by the kernel before
	 * the channel can be used.
	 **/
	if (bprint == NULL || retchan == NULL) { return ERROR_INVALID_ARG; };

	region0 = bprint->regionEnd0.region;
	region1 = bprint->regionEnd1.region;

	// We no longer allocate a new object. Just downcast.
	chan = static_cast<fplainn::D2DChannel *>(bprint);

	ret0 = region0->parent->addChannel(chan);
	if (region0->parent != region1->parent)
		{ ret1 = region1->parent->addChannel(chan); }
	else { ret1 = ret0; };

	if (ret0 != ERROR_SUCCESS || ret1 != ERROR_SUCCESS)
	{
		printf(ERROR ZUDI"createChannel: Failed to add chan to "
			"one or both deviceInstance objects.\n");

		return ERROR_INITIALIZATION_FAILURE;
	};

	assert_fatal(chan->endpoints[0] != chan->endpoints[1]);

	ret0 = chan->endpoints[0]->anchor();
	ret1 = chan->endpoints[1]->anchor();
	if (ret0 != ERROR_SUCCESS || ret1 != ERROR_SUCCESS)
	{
		printf(ERROR ZUDI"createChannel: Failed to add endpoints of "
			"new channel to endpoint-objects.\n"
			"\tret0 %d: %s, ret1 %d: %s",
			ret0, strerror(ret0), ret1, strerror(ret1));

		return ERROR_INITIALIZATION_FAILURE;
	};

	*retchan = chan;
	return ERROR_SUCCESS;
}

error_t fplainn::Zudi::spawnInternalBindChannel(
	utf8Char *devPath, utf8Char *metaName, ubit16 regionIndex,
	udi_ops_vector_t *opsVector0, udi_ops_vector_t *opsVector1,
	fplainn::Endpoint **retendp1
	)
{
	fplainn::Region				*region0, *region1;
	processId_t				region0Tid=PROCID_INVALID;
	fplainn::Device				*dev;
	error_t					ret;
	fplainn::D2DChannel			*tmpret;

	if (retendp1 == NULL) { return ERROR_INVALID_ARG; };

printf(NOTICE"spawnIBopChan(%s, %d, %p, %p).\n",
	devPath, regionIndex, opsVector0, opsVector1);
	/* region1 is always the caller because internal bind channels are
	 * spawned by the secondary region thread.
	 **/
	region1 = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread()
		->getRegion();

	ret = floodplainn.getDevice(devPath, &dev);
	if (ret != ERROR_SUCCESS) { return ret; };

	// If we can't get primary region info, something is terribly wrong.
	assert_fatal(
		dev->instance->getRegionInfo(0, &region0Tid)
		== ERROR_SUCCESS);

	region0 = region1->thread->parent->getThread(region0Tid)->getRegion();

	IncompleteD2DChannel		*blueprint;

	blueprint = new IncompleteD2DChannel(
		metaName, Channel::BIND_CHANNEL_TYPE_INTERNAL, 0);

	if (blueprint == NULL || blueprint->initialize() != ERROR_SUCCESS)
	{
		printf(ERROR"spawnIbopChan %s,%d,%p,%p: Failed to alloc or "
			"initialize new channel object.\n",
			devPath, regionIndex, opsVector0, opsVector1);

		return ERROR_INITIALIZATION_FAILURE;
	};

	blueprint->endpoints[0]->anchor(region0, opsVector0, NULL);
	blueprint->endpoints[1]->anchor(region1, opsVector1, NULL);
	ret = createChannel(blueprint, &tmpret);
	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR"spawnIbopChan(%s, %d, %p, %p): createChannel "
			"call failed. Ret %d.\n",
			devPath, regionIndex, opsVector0, opsVector1, ret);

		return ret;
	};

	*retendp1 = tmpret->endpoints[1];
	return ERROR_SUCCESS;
}

error_t fplainn::Zudi::spawnChildBindChannel(
	utf8Char *parentDevPath, utf8Char *childDevPath, utf8Char *metaName,
	udi_ops_vector_t *opsVector, fplainn::Endpoint **retendp1
	)
{
	fplainn::Device				*parentDev, *childDev;
	fplainn::Device::ParentTag		*parentTag;
	fplainn::Driver::sMetalanguage		*parentMeta, *childMeta;
	fplainn::Driver::sChildBop		*parentCBop;
	fplainn::Driver::sParentBop		*childPBop;
	fplainn::Region				*parentRegion, *childRegion;
	processId_t				parentTid, childTid;
	fplainn::DriverInstance::sChildBop	*parentInstCBop;
	fplainn::D2DChannel			*tmpret;
	error_t					ret;

	if (retendp1 == NULL) { return ERROR_INVALID_ARG; };

	if (floodplainn.getDevice(parentDevPath, &parentDev) != ERROR_SUCCESS
		|| floodplainn.getDevice(childDevPath, &childDev) != ERROR_SUCCESS)
		{ return ERROR_INVALID_RESOURCE_NAME; };

	/* Both the parent device and the connecting child device must expose
	 * "meta" indexes for the specified metalanguage.
	 **/
	childMeta = childDev->driverInstance->driver->getMetalanguage(metaName);
	parentMeta = parentDev->driverInstance->driver->getMetalanguage(
		metaName);

	if (childMeta == NULL || parentMeta == NULL)
	{
		printf(WARNING FPLAINN"spawnCBindChan: meta %s not declared "
			"by one or both devices.\n",
			metaName);

		return ERROR_NON_CONFORMANT;
	};

	/* Furthermore, the parent must have a child bop for the meta, while the
	 * child must have a parent bop for the meta. If any of these conditions
	 * are not met, the connection cannot be established.
	 **/
	parentCBop = parentDev->driverInstance->driver->getChildBop(
		parentMeta->index);

	childPBop = childDev->driverInstance->driver->getParentBop(
		childMeta->index);

	if (parentCBop == NULL || childPBop == NULL)
	{
		printf(WARNING FPLAINN"spawnCBindChan: meta %s not exposed "
			"as parent/child bop by one or both devices.\n",
			metaName);

		return ERROR_UNSUPPORTED;
	};

	// Next, get the region thread IDs.
	if (parentDev->instance->getRegionInfo(
		parentCBop->regionIndex, &parentTid) != ERROR_SUCCESS
		|| childDev->instance->getRegionInfo(
			childPBop->regionIndex, &childTid) != ERROR_SUCCESS)
	{
		printf(ERROR FPLAINN"spawnCBindChan: Either parent or child's "
			"Bop region ID is invalid.\n");

		return ERROR_INVALID_RESOURCE_ID;
	};

	// Now get the thread objects using their thread IDs.
	parentRegion = processTrib.getThread(parentTid)->getRegion();
	childRegion = processTrib.getThread(childTid)->getRegion();
	if (parentRegion == NULL || childRegion == NULL)
	{
		printf(ERROR FPLAINN"spawnCBindChan: Either parent or child's "
			"region TID doesn't map to valid thread.\n");

		return ERROR_INVALID_RESOURCE_ID;
	};

	// Next, we need the parent's ops vector and context info.
	parentInstCBop = parentDev->driverInstance->getChildBop(
		parentMeta->index);

	if (parentInstCBop == NULL)
	{
		printf(ERROR FPLAINN"spawnCBindChan: Parent driver instance "
			"hasn't set its child bop vectors.\n");

		return ERROR_UNINITIALIZED;
	};

	// Finally, we need to know the ParentId for this parentBindChan.
	parentTag = childDev->getParentTag(parentDev);
	if (parentTag == NULL)
	{
		printf(ERROR"spawnCBindChan(%s,%s,%s): No parent tag for "
			"dev %s found in child dev %s.\n",
			parentDevPath, childDevPath, metaName,
			parentDevPath, childDevPath);

		return ERROR_NOT_FOUND;
	}

	// This should definitely not be the case by this point.
	assert_fatal(parentTag->id != 0);

	// Finally, create the blueprint and then call spawnChannel.
	fplainn::IncompleteD2DChannel		*blueprint;

	blueprint = new IncompleteD2DChannel(
		metaName, Channel::BIND_CHANNEL_TYPE_CHILD, 0);

	if (blueprint == NULL || blueprint->initialize() != ERROR_SUCCESS)
	{
		printf(ERROR"spawnCBindChan %s,%s,%s,%p: Failed to alloc or "
			"initialize new channel object.\n",
			parentDevPath, childDevPath, metaName, opsVector);

		return ERROR_INITIALIZATION_FAILURE;
	};

	blueprint->endpoints[1]->anchor(childRegion, opsVector, NULL);
	blueprint->endpoints[0]->anchor(
		parentRegion, parentInstCBop->opsVector, NULL);

	ret = createChannel(blueprint, &tmpret);
	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR"spawnCBindChan(%s,%s,%s,%p): spawnChannel call "
			"failed. Ret %d.\n",
			parentDevPath, childDevPath, metaName, opsVector, ret);

		return ret;
	};

	tmpret->parentId = parentTag->id;
	*retendp1 = tmpret->endpoints[1];
	return ERROR_SUCCESS;
}

error_t fplainn::Zudi::send(
	fplainn::Endpoint *_endp,
	udi_cb_t *gcb, va_list args, udi_layout_t *layouts[3],
	utf8Char *metaName, udi_index_t meta_ops_num, udi_index_t ops_idx,
	void *privateData
	)
{
	fplainn::Region			*region;
	fplainn::RegionEndpoint		*endp;
	Thread				*self;

	/**	EXPLANATION:
	 * In this one we need to do a little more work; we must get the
	 * region for the current thread (only driver processes should call this
	 * function), and then we can proceed.
	 **/
	endp = static_cast<fplainn::RegionEndpoint *>(_endp);
	self = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();
	region = self->getRegion();

	if (self->parent->getType() != ProcessStream::DRIVER)
		{ return ERROR_UNAUTHORIZED; };

	if (!region->endpoints.checkForItem(endp))
	{
		fplainn::Channel	*chan;

		/* In the case of a udi_channel_event_complete response from
		 * the driver across the MA channel, the MA channel may not be
		 * connected to the region that is calling across it. Any
		 * secondary region may need to call over the MA channel.
		 *
		 * We do an extra, DeviceInstance-wide check for the endpoint
		 * if the first region-specific check fails.
		 *
		 * If the endpoint is the MA channel, we should find it...AND
		 * its metaName should also be "udi_mgmt". The MA channel is
		 * also ALWAYS a child-bind channel. This is how we
		 * identify the MA channel.
		 **/
		chan = region->parent->getChannelByEndpoint(endp);
		if (chan == NULL
			|| chan->bindChannelType != fplainn::Channel::BIND_CHANNEL_TYPE_CHILD
			|| strncmp8(
				chan->metaName, CC"udi_mgmt",
				DRIVER_METALANGUAGE_MAXLEN) != 0)
		{
			return ERROR_INVALID_RESOURCE_HANDLE;
		};
	};

	return fplainn::sChannelMsg::send(
		endp,
		gcb, args, layouts,
		metaName, meta_ops_num, ops_idx, privateData);
}
