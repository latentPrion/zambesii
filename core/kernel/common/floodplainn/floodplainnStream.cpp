
#include <__kstdlib/__kclib/string8.h>
#include <kernel/common/thread.h>
#include <kernel/common/process.h>
#include <kernel/common/floodplainn/floodplainn.h>
#include <kernel/common/floodplainn/floodplainnStream.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


error_t FloodplainnStream::initialize(void)
{
	error_t			ret;

	ret = metaConnections.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	ret = zkcmConnections.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	ret = endpoints.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	return ERROR_SUCCESS;
}

error_t FloodplainnStream::connect(
	utf8Char *devName, utf8Char *metaName,
	udi_ops_vector_t *ops_vector, void *endpPrivateData1,
	uarch_t, fplainn::Endpoint **retendp)
{
	error_t					ret;
	fplainn::Device				*dev;
	fplainn::Driver::sChildBop		*devCbop;
	fplainn::Driver::sMetalanguage		*devMeta;
	fplainn::Region				*devRegion;
	fplainn::DriverInstance::sChildBop	*drvInstCbop;
	fplainn::D2SChannel			*tmpretchan;

	/**	EXPLANATION:
	 * This is called to create an initial bind channel between a userspace
	 * process and a running device instance.
	 *
	 * Essentially, we delegate most of the work to
	 * fplainn::Channel::udi_channel_spawn(). When Channel::udi_ch_sp()
	 * returns a successful spawn, it will then add the successfully spawned
	 * channel to our list of channels that are connected to this process.
	 *
	 **	SEQUENCE:
	 * Get the device from the "devName". Now go through its childbops. For
	 * each cbop, get the meta-index. With the meta-index, lookup the meta
	 * name for that meta-index. If it matches, then we know we have a
	 * cbop for the requested metalanguage.
	 *
	 * Use the cbop that matched.
	 * 	c_bop <meta_index> <region_index> <ops_vector>
	 *
	 * Use the region_index to determine which region the new channel must
	 * be bound to. Use the region's init_context to determine what the
	 * initial channel_context for the device end should be.
	 *
	 * Spawn the FStream end ignoring the channel_context and ops_index.
	 * Done with the channel itself at this point.
	 *
	 * Now add the new channel to the deviceinstance, region end and the
	 * FStream end.
	 **/
	if (devName == NULL || metaName == NULL || retendp == NULL)
		{ return ERROR_INVALID_ARG; }

	ret = floodplainn.getDevice(devName, &dev);
	if (ret != ERROR_SUCCESS) { return ret; };

	// Search for a meta that matches "metaName" passed to us.
	devMeta = dev->driverInstance->driver->getMetalanguage(metaName);
	if (devMeta == NULL)
	{
		printf(ERROR FPSTREAM"%d: connect %s %s: Device does not "
			"have meta for %s.\n",
			this->id, devName, metaName, metaName);

		return ERROR_NO_MATCH;
	};

	// Scan through cbops to find one that has the meta's metaIndex.
	devCbop = dev->driverInstance->driver->getChildBop(devMeta->index);
	if (devCbop == NULL)
	{
		printf(ERROR FPSTREAM"%d: connect %s %s: Device does not "
			"expose cbop for %s.\n",
			this->id, devName, metaName, metaName);

		return ERROR_NOT_FOUND;
	};

	// Get the region for that the child bind channel should anchor to.
	devRegion = dev->instance->getRegion(devCbop->regionIndex);
	if (devRegion == NULL)
	{
		printf(ERROR FPSTREAM"%d: connect %s %s: Cbop for meta %s "
			"references inexistent region %d.\n",
			this->id, devName, metaName, metaName,
			devCbop->regionIndex);

		return ERROR_INVALID_RESOURCE_ID;
	};

	drvInstCbop = dev->driverInstance->getChildBop(devMeta->index);
	if (drvInstCbop == NULL)
	{
		printf(ERROR FPSTREAM"%d: connect %s %s: Cbop for meta %s "
			"unable to be located in deviceInstance.\n",
			this->id, devName, metaName, metaName);

		return ERROR_UNKNOWN;
	};

	/* We now have all the information we need about the target device.
	 * Begin gathering required information for the Stream end of the
	 * channel.
	 **/
	fplainn::IncompleteD2SChannel		*blueprint;

	blueprint = new fplainn::IncompleteD2SChannel(metaName, 0);
	if (blueprint == NULL || blueprint->initialize() != ERROR_SUCCESS)
	{
		printf(ERROR FPSTREAM"%d: connect %s %s: failed to alloc or "
			"initialize new channel object.\n",
			this->id, devName, metaName);

		return ERROR_INITIALIZATION_FAILURE;
	};

	blueprint->endpoints[0]->anchor(devRegion, drvInstCbop->opsVector, NULL);
	blueprint->endpoints[1]->anchor(
		cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread(),
		ops_vector, endpPrivateData1);

	ret = createChannel(blueprint, &tmpretchan);
	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR FPSTREAM"0x%x: connect %s %s: createChannel() "
			"failed. Err %d %s.\n",
			this->id, devName, metaName, ret, strerror(ret));

		return ret;
	};

	*retendp = tmpretchan->endpoints[1];
	return ERROR_SUCCESS;
}

error_t FloodplainnStream::createChannel(
	fplainn::IncompleteD2SChannel *blueprint, fplainn::D2SChannel **retchan
	)
{
	error_t				ret0, ret1;
	fplainn::Region			*region;
	FloodplainnStream		*stream;
	fplainn::D2SChannel		*chan;

	if (retchan == NULL) { return ERROR_INVALID_ARG; };

	region = blueprint->regionEnd0.region;
	stream = &blueprint->fstreamEnd.thread->parent->floodplainnStream;

	// We no longer allocate a new object; just downcast.
	chan = static_cast<fplainn::D2SChannel *>(blueprint);

	// Only one device instance to add the channel to.
	ret0 = region->parent->addChannel(chan);
	if (ret0 != ERROR_SUCCESS) { return ret0; };

	ret0 = chan->endpoints[0]->anchor();
	ret1 = chan->endpoints[1]->anchor();
	if (ret0 != ERROR_SUCCESS || ret1 != ERROR_SUCCESS)
	{
		printf(ERROR FPSTREAM"%x: createChannel: Failed to add "
			"endpoints of "
			"new channel to endpoint-objects.\n"
			"\tret0 %d: %s, ret1 %d: %s.\n",
			stream->id, ret0, strerror(ret0), ret1, strerror(ret1));

		region->parent->removeChannel(chan);
		return ERROR_INITIALIZATION_FAILURE;
	};

	if (retchan != NULL) {
		*retchan = static_cast<fplainn::D2SChannel*>(chan);
	};

	return ERROR_SUCCESS;
}

error_t FloodplainnStream::send(
	fplainn::Endpoint *_endp,
//	fplainn::Endpoint *endp,
	udi_cb_t *gcb, udi_layout_t *layouts[3],
	utf8Char *metaName, udi_index_t meta_ops_num, udi_index_t ops_idx,
	void *privateData,
	...
	)

{
	Thread				*self;
	fplainn::FStreamEndpoint	*endp;
	va_list				args;

	/**	EXPLANATION:
	 * We use the channel pointer inside of the GCB to know which channel
	 * endpoint we are meant to send this message over. We assume that
	 * the data pointer given to us is the beginning of the GCB for the
	 * marshalled data.
	 *
	 **	SYNOPSIS:
	 * We first check the channel pointer to ensure that it is a valid
	 * endpoint that is actually connected to this stream. After that, we
	 * needn't do much other than minor preparations before sending the
	 * message across the channel. Allocate the message
	 * (fplainn::sChannelMsg), then copy the marshaled data into the message
	 * memory. Then send the message across the channel.
	 **/
	va_start(args, privateData);
	endp = static_cast<fplainn::FStreamEndpoint *>(_endp);
	self = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	if (self->parent->getType() == ProcessStream::DRIVER)
		{ return ERROR_UNAUTHORIZED; };

	if (!endpoints.checkForItem(endp))
		{ return ERROR_INVALID_RESOURCE_HANDLE; };

	return fplainn::sChannelMsg::send(
		endp,
		gcb, args, layouts,
		metaName, meta_ops_num, ops_idx, privateData);
}
