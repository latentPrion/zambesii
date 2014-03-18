
#include <arch/debug.h>
#include <chipset/chipset.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kcxxlib/memory>
#include <__kclasses/debugPipe.h>
#include <kernel/common/zudiIndexServer.h>
#include <kernel/common/floodplainn/floodplainn.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/vfsTrib/vfsTrib.h>


static fplainn::deviceC		treeDev(CHIPSET_NUMA_SHBANKID);

error_t floodplainnC::initializeReq(initializeReqCallF *callback)
{
	fvfs::tagC		*root, *tmp;
	error_t			ret;

	/**	EXPLANATION:
	 * Create the four sub-trees and then exit.
	 **/
	ret = driverList.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	ret = treeDev.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	root = vfsTrib.getFvfs()->getRoot();

	ret = root->getInode()->createChild(CC"by-id", root, &treeDev, &tmp);
	if (ret != ERROR_SUCCESS) { return ret; };
	ret = root->getInode()->createChild(CC"by-name", root, &treeDev, &tmp);
	if (ret != ERROR_SUCCESS) { return ret; };
	ret = root->getInode()->createChild(CC"by-class", root, &treeDev, &tmp);
	if (ret != ERROR_SUCCESS) { return ret; };
	ret = root->getInode()->createChild(CC"by-path", root, &treeDev, &tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	zuiServer::setNewDeviceActionReq(
		zuiServer::NDACTION_INSTANTIATE,
		newSyscallback(
			&floodplainnC::initializeReq1, (void (*)())callback));

	return ERROR_SUCCESS;
}

void floodplainnC::initializeReq1(
	messageStreamC::iteratorS *res, void (*callback)()
	)
{
	initializeReqCallF	*_callback;

	if (res->header.error != ERROR_SUCCESS)
	{
		printf(ERROR FPLAINN"initialize: Failed to set new-device "
			"action.\n");
	};

	_callback = (initializeReqCallF *)callback;
	_callback(res->header.error);
}

error_t floodplainnC::findDriver(utf8Char *fullName, fplainn::driverC **retDrv)
{
	heapArrC<utf8Char>	tmpStr;
	void			*handle;
	fplainn::driverC	*tmpDrv;

	tmpStr = new utf8Char[DRIVER_FULLNAME_MAXLEN];

	if (tmpStr.get() == NULL) { return ERROR_MEMORY_NOMEM; };

	handle = NULL;
	driverList.lock();
	tmpDrv = driverList.getNextItem(&handle, PTRLIST_FLAGS_NO_AUTOLOCK);
	for (; tmpDrv != NULL;
		tmpDrv = driverList.getNextItem(
			&handle, PTRLIST_FLAGS_NO_AUTOLOCK))
	{
		uarch_t		len;

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

void floodplainnC::__kdriverEntry(void)
{
	threadC		*self;

	self = (threadC *)cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentTask();

	printf(NOTICE"Kernel driver: %s. Executing.\n", self->parent->fullName);
	taskTrib.dormant(self->getFullId());
}

static inline sarch_t isByIdPath(utf8Char *path)
{
	// Quick and dirty string parser to ensure the path is a by-id path.
	for (; *path == '/'; path++) {};
	if (strncmp8(path, CC"@f/", 3) == 0) { path += 3; };
	for (; *path == '/'; path++) {};
	if (strncmp8(path, CC"by-id", 5) != 0) { return 0; };
	return 1;
}

error_t floodplainnC::createDevice(
	utf8Char *parentId, numaBankId_t bid, ubit16 childId, ubit32 /*flags*/,
	fplainn::deviceC **device
	)
{
	error_t			ret;
	fvfs::tagC		*parentTag, *childTag;
	fplainn::deviceC	*newDev;

	/**	EXPLANATION:
	 * This function both creates the device, and creates its nodes in the
	 * by-id, by-path and by-name trees.
	 *
	 * The path here must be a by-id path.
	 **/
	if (parentId == NULL || device == NULL)
		{ return ERROR_INVALID_ARG; };

	if (!isByIdPath(parentId)) { return ERROR_INVALID_ARG_VAL; };

	// Get the parent node's VFS tag.
	ret = vfsTrib.getFvfs()->getPath(parentId, &parentTag);
	if (ret != ERROR_SUCCESS) { return ret; };

	newDev = new fplainn::deviceC(bid);
	if (newDev == NULL) { return ERROR_MEMORY_NOMEM; };
	ret = newDev->initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	utf8Char	tmpBuff[FVFS_TAG_NAME_MAXLEN];

	// Add the new device node to the parent node.
	snprintf(tmpBuff, FVFS_TAG_NAME_MAXLEN, CC"%d", childId);
	ret = parentTag->getInode()->createChild(
		tmpBuff, parentTag, newDev, &childTag);

	if (ret != ERROR_SUCCESS) { delete newDev; };
	*device = newDev;
	return ERROR_SUCCESS;
}

error_t floodplainnC::removeDevice(
	utf8Char *parentId, ubit32 childId, ubit32 /*flags*/
	)
{
	fvfs::tagC		*parentTag, *childTag;
	error_t			ret;

	/**	EXPLANATION:
	 * parentId must be a by-id path.
	 **/

	if (parentId == NULL) { return ERROR_INVALID_ARG; };

	if (!isByIdPath(parentId)) { return ERROR_INVALID_ARG_VAL; };
	ret = vfsTrib.getFvfs()->getPath(parentId, &parentTag);
	if (ret != ERROR_SUCCESS) { return ret; };

	utf8Char	tmpBuff[FVFS_TAG_NAME_MAXLEN];

	// Get the child tag.
	snprintf(tmpBuff, FVFS_TAG_NAME_MAXLEN, CC"%d", childId);
	childTag = parentTag->getInode()->getChild(tmpBuff);
	if (childTag == NULL) { return ERROR_NOT_FOUND; };

	// Don't remove the child if it has children.
	if (childTag->getInode()->dirs.getNItems() > 0)
		{ return ERROR_INVALID_OPERATION; };

	// Delete the device object it points to, then delete the child tag.
	delete childTag->getInode();
	parentTag->getInode()->removeChild(tmpBuff);

	return ERROR_SUCCESS;
}

error_t floodplainnC::getDevice(utf8Char *path, fplainn::deviceC **device)
{
	error_t		ret;
	fvfs::tagC	*tag;

	ret = vfsTrib.getFvfs()->getPath(path, &tag);
	if (ret != ERROR_SUCCESS) { return ret; };

	/* Do not allow callers to "getDevice" on the by-* containers.
	 * e.g: "by-id", "/by-id", "@f/by-id", etc.
	 **/
	if (tag->getInode() == &treeDev) { return ERROR_UNAUTHORIZED; };

	*device = tag->getInode();
	return ERROR_SUCCESS;
}

error_t floodplainnC::instantiateDeviceReq(utf8Char *path, void *privateData)
{
	zudiKernelCallMsgS			*request;
	fplainn::deviceC			*dev;
	error_t					ret;
	heapObjC<fplainn::deviceInstanceC>	devInst;

	if (path == NULL) { return ERROR_INVALID_ARG; };
	if (strnlen8(path, FVFS_PATH_MAXLEN) >= FVFS_PATH_MAXLEN)
		{ return ERROR_INVALID_RESOURCE_NAME; };

	ret = getDevice(path, &dev);
	if (ret != ERROR_SUCCESS) { return ret; };

	// Ensure that spawnDriver() has been called beforehand.
	if (!dev->driverDetected || dev->driverInstance == NULL)
		{ return ERROR_UNINITIALIZED; };

	if (dev->instance == NULL)
	{
		// Allocate the device instance.
		devInst = new fplainn::deviceInstanceC(dev);
		if (devInst == NULL) { return ERROR_MEMORY_NOMEM; };

		ret = devInst->initialize();
		if (ret != ERROR_SUCCESS) { return ret; };

		ret = dev->driverInstance->addHostedDevice(path);
		if (ret != ERROR_SUCCESS) { return ret; };
	};

	request = new zudiKernelCallMsgS(
		dev->driverInstance->pid,
		MSGSTREAM_SUBSYSTEM_ZUDI,
		MSGSTREAM_FPLAINN_ZUDI___KCALL,
		sizeof(*request), 0, privateData);

	if (request == NULL)
	{
		dev->driverInstance->removeHostedDevice(path);
		return ERROR_MEMORY_NOMEM;
	};

	// Assign the device instance to dev->instance and release the mem.
	request->set(path, zudiKernelCallMsgS::CMD_INSTANTIATE_DEVICE);
	if (dev->instance == NULL) { dev->instance = devInst.release(); };
	return messageStreamC::enqueueOnThread(
		request->header.targetId, &request->header);
}

void floodplainnC::instantiateDeviceAck(
	processId_t targetId, utf8Char *path, error_t err, void *privateData
	)
{
	threadC					*currThread;
	floodplainnC::zudiKernelCallMsgS	*response;
	fplainn::deviceC			*dev;

	currThread = (threadC *)cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentTask();

	// Only driver processes can call this.
	if (currThread->parent->getType() != processStreamC::DRIVER)
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

	response = new zudiKernelCallMsgS(
		targetId,
		MSGSTREAM_SUBSYSTEM_ZUDI,
		MSGSTREAM_FPLAINN_ZUDI___KCALL,
		sizeof(*response), 0, privateData);

	if (response == NULL) { return; };

	response->set(path, zudiKernelCallMsgS::CMD_INSTANTIATE_DEVICE);
	response->header.error = err;

	messageStreamC::enqueueOnThread(
		response->header.targetId, &response->header);
}

error_t floodplainnC::enumerateBaseDevices(void)
{
	error_t				ret;
	fplainn::deviceC		*vchipset, *ramdisk;
	udi_instance_attr_list_t	tmp;

	ret = createDevice(
		CC"by-id", CHIPSET_NUMA_SHBANKID, 0, 0, &ramdisk);

	if (ret != ERROR_SUCCESS) { return ret; };

	ret = createDevice(
		CC"by-id", CHIPSET_NUMA_SHBANKID, 1, 0, &vchipset);

	if (ret != ERROR_SUCCESS) { return ret; };

	/* Create the attributes for the ramdisk.
	 */
	strcpy8(CC tmp.attr_name, CC"identifier");
	tmp.attr_type = UDI_ATTR_STRING;
	strcpy8(CC tmp.attr_value, CC"__kramdisk");
	ret = ramdisk->addEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	strcpy8(CC tmp.attr_name, CC"address_locator");
	tmp.attr_type = UDI_ATTR_STRING;
	strcpy8(CC tmp.attr_value, CC"__kramdisk0");
	ret = ramdisk->addEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	/* Create the attributes for the virtual-chipset.
	 */
	strcpy8(CC tmp.attr_name, CC"identifier");
	tmp.attr_type = UDI_ATTR_STRING;
	strcpy8(CC tmp.attr_value, CC"__kvirtual-chipset");
	ret = vchipset->addEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	strcpy8(CC tmp.attr_name, CC"address_locator");
	tmp.attr_type = UDI_ATTR_STRING;
	strcpy8(CC tmp.attr_value, CC"__kvchipset0");
	ret = vchipset->addEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	ramdisk->bankId = vchipset->bankId = CHIPSET_NUMA_SHBANKID;
	return ERROR_SUCCESS;
}

error_t floodplainnC::enumerateChipsets(void)
{
	error_t				ret;
	fplainn::deviceC		*chipset;
	udi_instance_attr_list_t	tmp;

	ret = createDevice(
		CC"by-id", CHIPSET_NUMA_SHBANKID, 2, 0, &chipset);

	if (ret != ERROR_SUCCESS) { return ret; };

	/* Generic enumeration attributes.
	 **/
	strcpy8(CC tmp.attr_name, CC"identifier");
	tmp.attr_type = UDI_ATTR_STRING;
	strcpy8(CC tmp.attr_value, CC"ibm-pc");
	ret = chipset->addEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	strcpy8(CC tmp.attr_name, CC"address_locator");
	tmp.attr_type = UDI_ATTR_STRING;
	strcpy8(CC tmp.attr_value, CC"chipset0");
	ret = chipset->addEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	/* zbz_root enumeration attributes.
	 **/
	strcpy8(CC tmp.attr_name, CC"bus_type");
	tmp.attr_type = UDI_ATTR_STRING;
	strcpy8(CC tmp.attr_value, CC"zbz_root");
	ret = chipset->addEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	strcpy8(CC tmp.attr_name, CC"zbz_root_chipset_short_name");
	tmp.attr_type = UDI_ATTR_STRING;
	// strcpy8(CC tmp.attr_value, CC CHIPSET_SHORT_STRING);
	strcpy8(CC tmp.attr_value, CC"IBM-PC");
	ret = chipset->addEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	strcpy8(CC tmp.attr_name, CC"zbz_root_chipset_long_name");
	tmp.attr_type = UDI_ATTR_STRING;
	// strcpy8(CC tmp.attr_value, CC CHIPSET_LONG_STRING);
	strcpy8(CC tmp.attr_value, CC"IBM-PC-compatible-chipset");
	ret = chipset->addEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	strcpy8(CC tmp.attr_name, CC"zbz_root_chipset_using_smp");
	tmp.attr_type = UDI_ATTR_BOOLEAN;
	tmp.attr_value[0] = cpuTrib.usingChipsetSmpMode();
	ret = chipset->addEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	strcpy8(CC tmp.attr_name, CC"zbz_root_chipset_info0");
	tmp.attr_type = UDI_ATTR_STRING;
	tmp.attr_value[0] = '\0';
	ret = chipset->addEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	strcpy8(CC tmp.attr_name, CC"zbz_root_chipset_info1");
	tmp.attr_type = UDI_ATTR_STRING;
	tmp.attr_value[0] = '\0';
	ret = chipset->addEnumerationAttribute(&tmp);

	chipset->bankId = CHIPSET_NUMA_SHBANKID;
	return ERROR_SUCCESS;
}

const driverInitEntryS *floodplainnC::findDriverInitInfo(utf8Char *shortName)
{
	const driverInitEntryS	*tmp;

	for (tmp=driverInitInfo; tmp->udi_init_info != NULL; tmp++) {
		if (!strcmp8(tmp->shortName, shortName)) { return tmp; };
	};

	return NULL;
}

const metaInitEntryS *floodplainnC::findMetaInitInfo(utf8Char *shortName)
{
	const metaInitEntryS		*tmp;

	for (tmp=metaInitInfo; tmp->udi_meta_info != NULL; tmp++) {
		if (!strcmp8(tmp->shortName, shortName)) { return tmp; };
	};

	return NULL;
}

static error_t udi_mgmt_calls_prep(
	utf8Char *callerName, utf8Char *devPath, fplainn::deviceC **dev,
	heapObjC<floodplainnC::zudiMgmtCallMsgS> *request,
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

	*request = new floodplainnC::zudiMgmtCallMsgS(
		devPath, primaryRegionTid,
		MSGSTREAM_SUBSYSTEM_ZUDI, MSGSTREAM_FPLAINN_ZUDI_MGMT_CALL,
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
	udi_cb_t *cb, fplainn::deviceC *dev, void *privateData
	)
{
	cb->channel = NULL;
	cb->context = dev->instance->mgmtChannelContext;
	cb->scratch = NULL;
	cb->initiator_context = privateData;
	cb->origin = NULL;
}

void floodplainnC::udi_usage_ind(
	utf8Char *devPath, udi_ubit8_t usageLevel, void *privateData
	)
{
	heapObjC<zudiMgmtCallMsgS>		request;
	udi_usage_cb_t				*cb;
	fplainn::deviceC			*dev;

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
	if (messageStreamC::enqueueOnThread(
		request->header.targetId, &request->header) == ERROR_SUCCESS)
		{ request.release(); };
}

void floodplainnC::udi_usage_res(
	utf8Char *devicePath, processId_t targetTid, void *privateData
	)
{
	fplainn::deviceC		*dev;
	floodplainnC::zudiMgmtCallMsgS	*response;

	response = new zudiMgmtCallMsgS(
		devicePath, targetTid,
		MSGSTREAM_SUBSYSTEM_ZUDI, MSGSTREAM_FPLAINN_ZUDI_MGMT_CALL,
		sizeof(*response), 0, privateData);

	if (response == NULL)
	{
		printf(FATAL FPLAINN"usage_res: failed to alloc response "
			"message. Thread 0x%x may be stalled waiting for ever\n",
			targetTid);

		return;
	};

	if (messageStreamC::enqueueOnThread(
		response->header.targetId, &response->header) != ERROR_SUCCESS)
		{ delete response; };
}

void floodplainnC::udi_enumerate_req(
	utf8Char *, udi_ubit8_t /*enumerateLevel*/, void * /*privateData*/
	)
{
}

void floodplainnC::udi_devmgmt_req(
	utf8Char *, udi_ubit8_t /*op*/, udi_ubit8_t /*parentId*/,
	void * /*privateData*/
	)
{
}

void floodplainnC::udi_final_cleanup_req(utf8Char *, void * /*privateData*/)
{
}
