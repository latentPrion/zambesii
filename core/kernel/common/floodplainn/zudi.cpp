
#include <__kstdlib/__kcxxlib/memory>
#include <kernel/common/process.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
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

void fplainn::Zudi::udi_usage_ind(
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

void fplainn::Zudi::udi_usage_res(
	utf8Char *devicePath, processId_t targetTid, void *privateData
	)
{
	fplainn::Zudi::sMgmtCallMsg	*response;

	response = new fplainn::Zudi::sMgmtCallMsg(
		devicePath, targetTid,
		MSGSTREAM_SUBSYSTEM_ZUDI, MSGSTREAM_ZUDI_MGMT_CALL,
		sizeof(*response), 0, privateData);

	if (response == NULL)
	{
		printf(FATAL FPLAINN"usage_res: failed to alloc response "
			"message. Thread 0x%x may be stalled waiting for ever\n",
			targetTid);

		return;
	};

	if (MessageStream::enqueueOnThread(
		response->header.targetId, &response->header) != ERROR_SUCCESS)
		{ delete response; };
}

void fplainn::Zudi::udi_enumerate_req(
	utf8Char *, udi_ubit8_t /*enumerateLevel*/, void * /*privateData*/
	)
{
}

void fplainn::Zudi::udi_devmgmt_req(
	utf8Char *, udi_ubit8_t /*op*/, udi_ubit8_t /*parentId*/,
	void * /*privateData*/
	)
{
}

void fplainn::Zudi::udi_final_cleanup_req(utf8Char *, void * /*privateData*/)
{
}
