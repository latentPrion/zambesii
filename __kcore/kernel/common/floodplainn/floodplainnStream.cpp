
#include <debug.h>
#include <__kstdlib/__kclib/string8.h>
#include <kernel/common/thread.h>
#include <kernel/common/process.h>
#include <kernel/common/processTrib/processTrib.h>
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

	ret = scatterGatherLists.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	return ERROR_SUCCESS;
}

error_t FloodplainnStream::getParentConstraints(
	ubit16 parentId, fplainn::dma::constraints::Compiler *ret
	)
{
	Thread					*self;
	fplainn::Device				*currDev;
	fplainn::Device::ParentTag		*parentTag;
	uarch_t					rwflags;;

	if (ret == NULL) { return ERROR_INVALID_ARG; }

	self = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	if ((udi_buf_path_t)(uintptr_t)parentId == UDI_NULL_BUF_PATH)
	{
		printf(WARNING FPSTREAM_ID "getParentConstraints(NULL_PATH): "
			"Invalid parentId.\n",
			self->getFullId(), parentId);
		return ERROR_INVALID_ARG_VAL;
	}


	if (self->parent->getType() != ProcessStream::DRIVER)
	{
		printf(ERROR FPSTREAM_ID"getParentConstraints(%d): Caller is "
			"not a driver process.\n",
			self->getFullId(), parentId);
		return ERROR_UNAUTHORIZED;
	}

	currDev = self->getRegion()->parent->device;
	parentTag = currDev->getParentTag(parentId);
	if (parentTag == NULL)
	{
		printf(ERROR FPSTREAM_ID"getParentConstraints(%d): Parent "
			"not recognized.\n",
			self->getFullId(), parentId);

		return ERROR_NOT_FOUND;
	}

	// Still copy it regardless of whether or not it's valid.
	parentTag->compiledConstraints.lock.readAcquire(&rwflags);
	*ret = parentTag->compiledConstraints.rsrc;
	parentTag->compiledConstraints.lock.readRelease(rwflags);

	if (!ret->isValid())
	{
		printf(WARNING FPSTREAM_ID"getParentConstraints(%d): "
			"Constraints object uninitialized. (Parent is %s).\n",
			self->getFullId(), parentId, parentTag->tag->getName());

		return ERROR_UNINITIALIZED;
	}

	return ERROR_SUCCESS;
}

error_t FloodplainnStream::setChildConstraints(
	ubit32 childId, fplainn::dma::constraints::Compiler *compiledConstraints
	)
{
	Thread				*self;
	fplainn::Device			*currDev;
	fvfs::Tag			*childDevTag;
	fplainn::Device::ParentTag	*parentTag;
	uarch_t				nParentTags;

	if (compiledConstraints == NULL) { return ERROR_INVALID_ARG; }

	self = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	if (self->parent->getType() != ProcessStream::DRIVER)
	{
		printf(ERROR FPSTREAM_ID"setChildConstraints(%d): Caller is "
			"not a driver.\n",
			self->getFullId(), childId);

		return ERROR_UNAUTHORIZED;
	}

	currDev = self->getRegion()->parent->device;

	utf8Char		tmpBuff[FVFS_TAG_NAME_MAXLEN];

	snprintf(tmpBuff, FVFS_TAG_NAME_MAXLEN, CC"%d", childId);
	childDevTag = currDev->getChild(tmpBuff);
	if (childDevTag == NULL)
	{
		// The child must already be known to the kernel.
		printf(ERROR FPSTREAM_ID"setChildConstraints(%d): Unknown "
			"childID.\n",
			self->getFullId(), childId);

		return ERROR_NOT_FOUND;
	}

	/* Problem: we don't know what our own parentId is as it was reported to
	 * the child by the kernel. We'll have to just peek into each parentTag
	 * of the child until we find the one that contains a pointer to
	 * ourself.
	 */
	parentTag = NULL;
	nParentTags = childDevTag->getInode()->getNParentTags();

	for (uarch_t i=0; i<nParentTags; i++)
	{
		fplainn::Device::ParentTag	*curr;

		curr = childDevTag->getInode()->indexedGetParentTag(i);
		if (curr == NULL || !curr->isValid()) { continue; }

		if (curr->tag->getInode() != currDev) { continue; }
		parentTag = curr;
		break;
	}

	if (parentTag == NULL)
	{
		printf(FATAL FPSTREAM_ID"setParentConstraints(%d): ChildId "
			"exists from parent's PoV, but child has no parentTag "
			"pointing to parent!\n",
			self->getFullId(), childId);

		return ERROR_UNKNOWN;
	}

	parentTag->compiledConstraints.lock.writeAcquire();
	parentTag->compiledConstraints.rsrc = *compiledConstraints;
	parentTag->compiledConstraints.lock.writeRelease();
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

	blueprint = new fplainn::IncompleteD2SChannel(
		metaName, fplainn::Channel::BIND_CHANNEL_TYPE_CHILD, 0);

	if (blueprint == NULL || blueprint->initialize() != ERROR_SUCCESS)
	{
		printf(ERROR FPSTREAM"%d: connect %s %s: failed to alloc or "
			"initialize new channel object.\n",
			this->id, devName, metaName);

		if (blueprint != NULL) {
			delete blueprint;
		}

		return ERROR_INITIALIZATION_FAILURE;
	};

	blueprint->endpoints[0]->anchor(devRegion, drvInstCbop->opsVector, NULL);
	blueprint->endpoints[1]->anchor(
		cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread(),
		ops_vector, endpPrivateData1);

	ret = createChannel(blueprint, &tmpretchan);
	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR FPSTREAM"%x: connect %s %s: createChannel() "
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

status_t FloodplainnStream::allocateScatterGatherList(
	fplainn::dma::ScatterGatherList **retlist
	)
{
	status_t	newId;
	error_t 	err;
	fplainn::dma::ScatterGatherList			*newObj=NULL;

	/**	EXPLANATION:
	 *
	 *
	 * Returns positive integer ID greater than 0 if successful. Else
	 * returns an error_t error value.
	 **/

	scatterGatherLists.lock();

	newId = scatterGatherLists.unlocked_getNextValue();
	if (newId < 0)
	{
		uarch_t					currentNIndexes;

		currentNIndexes = scatterGatherLists.unlocked_getNIndexes();
		err = scatterGatherLists.resizeToHoldIndex(
			currentNIndexes
				+ FPLAINN_STREAM_SCGTH_LIST_GROWTH_STRIDE - 1,
			ResizeableIdArray<typename fplainn::dma::ScatterGatherList>::RTHI_FLAGS_UNLOCKED);

		if (err != ERROR_SUCCESS)
		{
			scatterGatherLists.unlock();

			printf(ERROR"allocScgthList: Failed to resize internal "
				"array.\n");

			return err;
		}

		assert_fatal(
			scatterGatherLists.unlocked_getNIndexes()
				== currentNIndexes
				+ FPLAINN_STREAM_SCGTH_LIST_GROWTH_STRIDE);

		scatterGatherLists.idCounter.setMaxVal(
			currentNIndexes
				+ FPLAINN_STREAM_SCGTH_LIST_GROWTH_STRIDE - 1);

		/* Zero out the newly allocated memory. */
		newObj = &scatterGatherLists[currentNIndexes];
		memset(
			newObj, 0, sizeof(*newObj)
				* FPLAINN_STREAM_SCGTH_LIST_GROWTH_STRIDE);

		// If it fails now, something really weird happened.
		newId = scatterGatherLists.unlocked_getNextValue();
	}

	scatterGatherLists.unlock();

	if (newId < 0)
	{
		printf(FATAL"allocScgthList: Failed get ID for new scgth list "
			"even though array resize worked.\n");

		return ERROR_UNKNOWN;
	}

	if (newObj == NULL) {
		newObj = &scatterGatherLists[newId];
	}

	new (newObj) fplainn::dma::ScatterGatherList;
	err = newObj->initialize();
	if (err != ERROR_SUCCESS)
	{
		printf(ERROR FPSTREAM"%x: allocScgthList: Failed to initialize "
			"new SGList.\n",
			this->id);

		return err;
	}

	if (retlist != NULL) { *retlist = newObj; }
	return newId;
}

error_t FloodplainnStream::resizeScatterGatherList(sarch_t id, uarch_t nFrames)
{
	fplainn::dma::ScatterGatherList		*sgl;

	if (nFrames == 0) { return ERROR_SUCCESS; }

	sgl = getScatterGatherList(id);
	if (sgl == NULL)
	{
		printf(WARNING"resizeSGList(%d): ID indexes to a "
			"blank/invalid array index.\n",
			id);

		return ERROR_NOT_FOUND;
	}

	return sgl->resize(nFrames);
}

sbit8 FloodplainnStream::releaseScatterGatherList(
	sarch_t id, sbit8 justTransfer
	)
{
	fplainn::dma::ScatterGatherList		*sgl;

	sgl = getScatterGatherList(id);
	if (sgl == NULL)
	{
		printf(WARNING"releaseSGList(%d): ID indexes to a "
			"blank/invalid array index.\n",
			id);

		return 0;
	}

	if (!justTransfer)
	{
		// Destroy the object.
		sgl->~ScatterGatherList();
	}
	else {
		sgl->destroySGList(0);
	}

	/* We can assume that the userspace program will not attempt to double-
	 * free the array index?
	 *
	 * Acquire the lock still in case another thread within the owning
	 * Stream attempts to call allocateScatterGatherList while we are
	 * performing memset here.
	 *
	 * The results shouldn't be catastrophic but just in case.
	 **/
	scatterGatherLists.lock();
	memset(sgl, 0, sizeof(*sgl));
	scatterGatherLists.unlock();
	return 1;
}

status_t FloodplainnStream::transferScatterGatherList(
	processId_t destStreamId, sarch_t srcListId, uarch_t flags
	)
{
	ProcessStream						*destProcess;
	fplainn::dma::ScatterGatherList				*srcList,
								*newDestSGList;
	sarch_t							newDestId;

	/**	EXPLANATION:
	 * Transfer a scatter gather list from one process to another.
	 *
	 * This process doesn't actually transfer the frames in the scatter
	 * gather list, but just the metadata describing the frames.
	 *
	 * SGLists cannot be copied; only transfered or deallocated. It is
	 * because of this restriction that we can track and garbage collect
	 * them without refcounting.
	 *
	 *	CAVEAT:
	 * This method is untested.
	 **/
	destProcess = processTrib.getStream(destStreamId);
	if (destProcess == NULL) { return ERROR_INVALID_ARG_VAL; };

	srcList = getScatterGatherList(srcListId);
	if (srcList == NULL) { return ERROR_INVALID_ARG_VAL; };

	newDestId = destProcess->floodplainnStream.allocateScatterGatherList(
		&newDestSGList);

	if (newDestId < 0)
	{
		printf(ERROR"transferSGList(%x, %d, %x): Failed to alloc slot "
			"in target stream.\n",
			destStreamId, srcListId, flags);

		return newDestId;
	};

	/* We now have both the list and the dest stream. Just transfer. The
	 * easiest way to do this is to call allocScatterGatherList() on the
	 * target FloodplainnStream, then overwrite the returned index number
	 * with the source object we wish to transfer.
	 *
	 * We should not need to lock the dest list while doing this since
	 * the new slot we just allocated should not be used by anyone other
	 * than us, the kernel, since we haven't given it out to anybody else
	 * yet.
	 **/
	*newDestSGList = *srcList;
	releaseScatterGatherList(srcListId, 1);

	/**	TODO:
	 * Now take any tail-end actions required to preserve the API's expected
	 * behaviour, such as umapping or fakemapping, etc.
	 **/
	return newDestId;
}
