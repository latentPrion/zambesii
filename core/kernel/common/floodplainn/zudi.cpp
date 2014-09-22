
#include <__kstdlib/__kcxxlib/memory>
#include <__kstdlib/__kclib/assert.h>
#include <kernel/common/process.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/floodplainn/floodplainn.h>
#include <kernel/common/floodplainn/zudi.h>
#include <kernel/common/floodplainn/device.h>


error_t fplainn::Zudi::initialize(void)
{
	error_t		ret;

	ret = driverList.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	return ERROR_SUCCESS;
}

error_t fplainn::Zudi::findDriver(utf8Char *fullName, fplainn::Driver **retDrv)
{
	HeapArr<utf8Char>	tmpStr;
	fplainn::Driver		*tmpDrv;

	tmpStr = new utf8Char[DRIVER_FULLNAME_MAXLEN];

	if (tmpStr.get() == NULL) { return ERROR_MEMORY_NOMEM; };

	PtrList<fplainn::Driver>::Iterator		it =
		driverList.begin(PTRLIST_FLAGS_NO_AUTOLOCK);

	driverList.lock();
	for (; it != driverList.end(); ++it)
	{
		uarch_t		len;

		tmpDrv = *it;

		len = strlen8(tmpDrv->basePath);
		strcpy8(tmpStr.get(), tmpDrv->basePath);
		if (len > 0 && tmpStr[len - 1] != '/')
			{ strcat8(tmpStr.get(), CC"/"); };

		strcat8(tmpStr.get(), tmpDrv->shortName);

		if (strcmp8(tmpStr.get(), fullName) == 0)
		{
			driverList.unlock();
			*retDrv = tmpDrv;
			return ERROR_SUCCESS;
		};
	};

	driverList.unlock();
	return ERROR_NO_MATCH;
}

error_t fplainn::Zudi::instantiateDeviceReq(utf8Char *path, void *privateData)
{
	fplainn::Zudi::sKernelCallMsg			*request;
	fplainn::Device				*dev;
	error_t					ret;
	HeapObj<fplainn::DeviceInstance>	devInst;

	if (path == NULL) { return ERROR_INVALID_ARG; };
	if (strnlen8(path, FVFS_PATH_MAXLEN) >= FVFS_PATH_MAXLEN)
		{ return ERROR_INVALID_RESOURCE_NAME; };

	ret = floodplainn.getDevice(path, &dev);
	if (ret != ERROR_SUCCESS) { return ret; };

	// Ensure that spawnDriver() has been called beforehand.
	if (!dev->driverDetected || dev->driverInstance == NULL)
		{ return ERROR_UNINITIALIZED; };

	if (dev->instance == NULL)
	{
		// Allocate the device instance.
		devInst = new fplainn::DeviceInstance(dev);
		if (devInst == NULL) { return ERROR_MEMORY_NOMEM; };

		ret = devInst->initialize();
		if (ret != ERROR_SUCCESS) { return ret; };

		ret = dev->driverInstance->addHostedDevice(path);
		if (ret != ERROR_SUCCESS) { return ret; };
	};

	request = new fplainn::Zudi::sKernelCallMsg(
		dev->driverInstance->pid,
		MSGSTREAM_SUBSYSTEM_ZUDI, MSGSTREAM_ZUDI___KCALL,
		sizeof(*request), 0, privateData);

	if (request == NULL)
	{
		dev->driverInstance->removeHostedDevice(path);
		return ERROR_MEMORY_NOMEM;
	};

	// Assign the device instance to dev->instance and release the mem.
	request->set(path, fplainn::Zudi::sKernelCallMsg::CMD_INSTANTIATE_DEVICE);
	if (dev->instance == NULL) { dev->instance = devInst.release(); };
	return MessageStream::enqueueOnThread(
		request->header.targetId, &request->header);
}

void fplainn::Zudi::instantiateDeviceAck(
	processId_t targetId, utf8Char *path, error_t err, void *privateData
	)
{
	Thread				*currThread;
	fplainn::Zudi::sKernelCallMsg	*response;
	fplainn::Device			*dev;

	currThread = (Thread *)cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentThread();

	// Only driver processes can call this.
	if (currThread->parent->getType() != ProcessStream::DRIVER)
		{ return; };

	// If the instantiation didn't work, delete the instance object.
	if (err != ERROR_SUCCESS)
	{
		if (floodplainn.getDevice(path, &dev) != ERROR_SUCCESS)
		{
			printf(ERROR FPLAINN"instantiateDevAck: getDevice "
				"failed on %s.\n\tUnable to unset "
				"device->instance pointer.\n",
				path);

			return;
		};

		dev->driverInstance->removeHostedDevice(path);
		// Set the dev->instance pointer to NULL.
		delete dev->instance;
		dev->instance = NULL;
	};

	response = new fplainn::Zudi::sKernelCallMsg(
		targetId,
		MSGSTREAM_SUBSYSTEM_ZUDI, MSGSTREAM_ZUDI___KCALL,
		sizeof(*response), 0, privateData);

	if (response == NULL) { return; };

	response->set(path, fplainn::Zudi::sKernelCallMsg::CMD_INSTANTIATE_DEVICE);
	response->header.error = err;

	MessageStream::enqueueOnThread(
		response->header.targetId, &response->header);
}

const sDriverInitEntry *fplainn::Zudi::findDriverInitInfo(utf8Char *shortName)
{
	const sDriverInitEntry	*tmp;

	for (tmp=driverInitInfo; tmp->udi_init_info != NULL; tmp++) {
		if (!strcmp8(tmp->shortName, shortName)) { return tmp; };
	};

	return NULL;
}

const sMetaInitEntry *fplainn::Zudi::findMetaInitInfo(utf8Char *shortName)
{
	const sMetaInitEntry		*tmp;

	for (tmp=metaInitInfo; tmp->udi_meta_info != NULL; tmp++) {
		if (!strcmp8(tmp->shortName, shortName)) { return tmp; };
	};

	return NULL;
}

error_t fplainn::Zudi::createChannel(fplainn::IncompleteD2DChannel *bprint)
{
	HeapObj<fplainn::D2DChannel>	chan;
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
	if (bprint == NULL) { return ERROR_INVALID_ARG; };

	region0 = bprint->regionEnd0.region;
	region1 = bprint->regionEnd1.region;

	chan = new fplainn::D2DChannel(*bprint);
	if (chan == NULL) { return ERROR_MEMORY_NOMEM; };
	ret0 = chan->initialize();
	if (ret0 != ERROR_SUCCESS) { return ret0; };

	ret0 = region0->parent->addChannel(chan.get());
	if (region0->parent != region1->parent)
		{ ret1 = region1->parent->addChannel(chan.get()); }
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

	chan.release();
	return ERROR_SUCCESS;
}

error_t fplainn::Zudi::spawnInternalBindChannel(
	utf8Char *devPath, ubit16 regionIndex,
	udi_ops_vector_t *opsVector0, udi_ops_vector_t *opsVector1)
{
	fplainn::Region	*region0, *region1;
	udi_init_context_t			*rdata0=NULL, *rdata1=NULL;
	processId_t				region0Tid=PROCID_INVALID,
						dummy;
	fplainn::Device				*dev;
	error_t					ret;

printf(NOTICE"spawnIBopChan(%s, %d, 0x%p, 0x%p).\n",
	devPath, regionIndex, opsVector0, opsVector1);
	/* region1 is always the caller because internal bind channels are
	 * spawned by the secondary region thread.
	 **/
	region1 = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread()
		->getRegion();

	ret = floodplainn.getDevice(devPath, &dev);
	if (ret != ERROR_SUCCESS) { return ret; };

	if (dev->instance->getRegionInfo(regionIndex, &dummy, &rdata1)
		!= ERROR_SUCCESS)
		{ return ERROR_INVALID_ARG_VAL; };

	// If we can't get primary region info, something is terribly wrong.
	assert_fatal(
		dev->instance->getRegionInfo(0, &region0Tid, &rdata0)
		== ERROR_SUCCESS);

	region0 = region1->thread->parent->getThread(region0Tid)->getRegion();

	IncompleteD2DChannel		blueprint(0);

	blueprint.initialize();
	blueprint.endpoints[0]->anchor(region0, opsVector0, rdata0);
	blueprint.endpoints[1]->anchor(region1, opsVector1, rdata1);
	return createChannel(&blueprint);
}

error_t fplainn::Zudi::spawnChildBindChannel(
	utf8Char *parentDevPath, utf8Char *childDevPath, utf8Char *metaName,
	udi_ops_vector_t *opsVector
	)
{
	fplainn::Device				*parentDev, *childDev;
	fplainn::Driver::sMetalanguage		*parentMeta, *childMeta;
	fplainn::Driver::sChildBop		*parentCBop;
	fplainn::Driver::sParentBop		*childPBop;
	fplainn::Region	*parentRegion, *childRegion;
	processId_t				parentTid, childTid;
	udi_init_context_t			*parentRdata, *childRdata;
	fplainn::DriverInstance::sChildBop	*parentInstCBop;

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
		parentCBop->regionIndex, &parentTid, &parentRdata)
		!= ERROR_SUCCESS
		|| childDev->instance->getRegionInfo(
			childPBop->regionIndex, &childTid, &childRdata)
			!= ERROR_SUCCESS)
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

	// Finally, we need the parent's ops vector and context info.
	parentInstCBop = parentDev->driverInstance->getChildBop(
		parentMeta->index);

	if (parentInstCBop == NULL)
	{
		printf(ERROR FPLAINN"spawnCBindChan: Parent driver instance "
			"hasn't set its child bop vectors.\n");

		return ERROR_UNINITIALIZED;
	};

	// Finally, create the blueprint and then call spawnChannel.
	fplainn::IncompleteD2DChannel		blueprint(0);

	blueprint.initialize();
	blueprint.endpoints[1]->anchor(childRegion, opsVector, childRdata);
	blueprint.endpoints[0]->anchor(
		parentRegion, parentInstCBop->opsVector, parentRdata);

	return createChannel(&blueprint);
}

static error_t udi_mgmt_calls_prep(
	utf8Char *callerName, utf8Char *devPath, fplainn::Device **dev,
	HeapObj<fplainn::Zudi::sMgmtCallMsg> *request,
	void *privateData
	)
{
	processId_t		primaryRegionTid=PROCID_INVALID;
	udi_init_context_t	*dummy;

	if (floodplainn.getDevice(devPath, dev) != ERROR_SUCCESS)
	{
		printf(ERROR FPLAINN"%s: device %s doesn't exist.\n",
			callerName, devPath);

		return ERROR_INVALID_RESOURCE_NAME;
	};

	if (!(*dev)->driverDetected
		|| (*dev)->driverInstance == NULL || (*dev)->instance == NULL)
		{ return ERROR_UNINITIALIZED; };

	(*dev)->instance->getRegionInfo(0, &primaryRegionTid, &dummy);

	*request = new fplainn::Zudi::sMgmtCallMsg(
		devPath, primaryRegionTid,
		MSGSTREAM_SUBSYSTEM_ZUDI, MSGSTREAM_ZUDI_MGMT_CALL,
		sizeof(**request), 0, privateData);

	if (request->get() == NULL)
	{
		printf(ERROR FPLAINN"%s: Failed to alloc request.\n",
			callerName);

		return ERROR_MEMORY_NOMEM;
	};

	return ERROR_SUCCESS;
}

static void udi_usage_gcb_prep(
	udi_cb_t *cb, fplainn::Device *dev, void *privateData
	)
{
	cb->channel = NULL;
	cb->context = dev->instance->mgmtChannelContext;
	cb->scratch = NULL;
	cb->initiator_context = privateData;
	cb->origin = NULL;
}

void fplainn::Zudi::Management::udi_usage_ind(
	utf8Char *devPath, udi_ubit8_t usageLevel, void *privateData
	)
{
	HeapObj<fplainn::Zudi::sMgmtCallMsg>	request;
	udi_usage_cb_t				*cb;
	fplainn::Device				*dev;

	if (udi_mgmt_calls_prep(
		CC"udi_usage_ind", devPath, &dev, &request, privateData)
		!= ERROR_SUCCESS)
		{ return; };

	// Set the request block's parameters.
	request->set_usage_ind(usageLevel);

	// Next, fill in the control block.
	cb = &request->cb.ucb;
	cb->trace_mask = 0;
	cb->meta_idx = 0;

	/** FIXME:
	 * In this part here, we use the initiator_context member of the UDI
	 * control block to store very important continuation information for
	 * the driver process.
	 *
	 * A malicious driver can crash a whole driver process (and the device
	 * instances in it) by modifying this pointer. We need to find a way
	 * to make the kernel less dependent on this context pointer.
	 **/
	udi_usage_gcb_prep(&cb->gcb, dev, privateData);
	// Now send the request.
	if (MessageStream::enqueueOnThread(
		request->header.targetId, &request->header) == ERROR_SUCCESS)
		{ request.release(); };
}

void fplainn::Zudi::Management::udi_enumerate_req(
	utf8Char *, udi_ubit8_t /*enumerateLevel*/, void * /*privateData*/
	)
{
}

void fplainn::Zudi::Management::udi_devmgmt_req(
	utf8Char *, udi_ubit8_t /*op*/, udi_ubit8_t /*parentId*/,
	void * /*privateData*/
	)
{
}

void fplainn::Zudi::Management::udi_final_cleanup_req(utf8Char *, void * /*privateData*/)
{
}
