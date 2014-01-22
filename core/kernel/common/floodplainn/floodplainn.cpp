
#include <chipset/chipset.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kcxxlib/memory>
#include <__kclasses/debugPipe.h>
#include <kernel/common/zudiIndexServer.h>
#include <kernel/common/floodplainn/floodplainn.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/vfsTrib/vfsTrib.h>


fplainn::deviceC		byId(NUMABANKID_INVALID, 0, CC"by-id-tree"),
				byName(NUMABANKID_INVALID, 0, CC"by-name-tree"),
				byPath(NUMABANKID_INVALID, 0, CC"by-path-tree"),
				byClass(
					NUMABANKID_INVALID, 0,
					CC"by-class-tree");

error_t floodplainnC::initializeReq(initializeReqCallF *callback)
{
	fvfs::tagC		*root, *tmp;
	error_t			ret;

	/**	EXPLANATION:
	 * Create the four sub-trees and then exit.
	 **/
	ret = driverList.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	root = vfsTrib.getFvfs()->getRoot();

	ret = root->createChild(CC"by-id", &byId, &tmp);
	if (ret != ERROR_SUCCESS) { return ret; };
	ret = root->createChild(CC"by-name", &byName, &tmp);
	if (ret != ERROR_SUCCESS) { return ret; };
	ret = root->createChild(CC"by-class", &byClass, &tmp);
	if (ret != ERROR_SUCCESS) { return ret; };
	ret = root->createChild(CC"by-path", &byPath, &tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	zudiIndexServer::setNewDeviceActionReq(
		zudiIndexServer::NDACTION_INSTANTIATE,
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
	heapPtrC<utf8Char>	tmpStr;
	void			*handle;
	fplainn::driverC	*tmpDrv;

	tmpStr.useArrayDelete = 1;
	tmpStr = new utf8Char[
		ZUDI_DRIVER_BASEPATH_MAXLEN + ZUDI_DRIVER_SHORTNAME_MAXLEN];

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

	newDev = new fplainn::deviceC(bid, childId, NULL);
	if (newDev == NULL) { return ERROR_MEMORY_NOMEM; };

	utf8Char	tmpBuff[FVFS_TAG_NAME_MAXLEN];

	// Add the new device node to the parent node.
	snprintf(tmpBuff, FVFS_TAG_NAME_MAXLEN, CC"%d", childId);
	ret = parentTag->createChild(tmpBuff, newDev, &childTag);
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
	childTag = parentTag->getChild(tmpBuff);
	if (childTag == NULL) { return ERROR_NOT_FOUND; };

	// Don't remove the child if it has children.
	if (childTag->nDirs > 0) { return ERROR_INVALID_OPERATION; };

	// Delete the device object it points to, then delete the child tag.
	delete childTag->getDevice();
	parentTag->removeChild(tmpBuff);

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
	if (tag->getDevice() == &byId || tag->getDevice() == &byName
		|| tag->getDevice() == &byClass
		|| tag->getDevice() == &byPath)
	{
		return ERROR_UNAUTHORIZED;
	};

	*device = tag->getDevice();
	return ERROR_SUCCESS;
}

error_t floodplainnC::instantiateDeviceReq(utf8Char *path, void *privateData)
{
	instantiateDeviceMsgS		*request;
	fplainn::deviceC		*dev;
	error_t				ret;

	if (strlen8(path) >= ZUDIIDX_SERVER_MSG_DEVICEPATH_MAXLEN)
		{ return ERROR_INVALID_RESOURCE_NAME; };

	ret = getDevice(path, &dev);
	if (ret != ERROR_SUCCESS) { return ret; };

	// Ensure that spawnDriver() has been called beforehand.
	if (!dev->driverDetected || dev->driverInstance == NULL)
		{ return ERROR_UNINITIALIZED; };

	request = new instantiateDeviceMsgS(
		dev->driverInstance->pid,
		MSGSTREAM_SUBSYSTEM_FLOODPLAINN,
		MSGSTREAM_FPLAINN_INSTANTIATE_DEVICE_REQ,
		sizeof(*request), 0, privateData);

	if (request == NULL) { return ERROR_MEMORY_NOMEM; };

	strcpy8(request->path, path);
	return messageStreamC::enqueueOnThread(
		request->header.targetId, &request->header);
}

void floodplainnC::instantiateDeviceAck(
	processId_t targetId, utf8Char *path, error_t err, void *privateData
	)
{
	threadC					*currThread;
	floodplainnC::instantiateDeviceMsgS	*response;

	currThread = (threadC *)cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentTask();

	// Only driver processes can call this.
	if (currThread->parent->getType() != processStreamC::DRIVER)
		{ return; };

	response = new instantiateDeviceMsgS(
		targetId,
		MSGSTREAM_SUBSYSTEM_FLOODPLAINN,
		MSGSTREAM_FPLAINN_INSTANTIATE_DEVICE_REQ,
		sizeof(*response), 0, privateData);

	if (response == NULL) { return; };

	strcpy8(response->path, path);
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
	ret = ramdisk->setEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	strcpy8(CC tmp.attr_name, CC"address_locator");
	tmp.attr_type = UDI_ATTR_STRING;
	strcpy8(CC tmp.attr_value, CC"__kramdisk0");
	ret = ramdisk->setEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	/* Create the attributes for the virtual-chipset.
	 */
	strcpy8(CC tmp.attr_name, CC"identifier");
	tmp.attr_type = UDI_ATTR_STRING;
	strcpy8(CC tmp.attr_value, CC"__kvirtual-chipset");
	ret = vchipset->setEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	strcpy8(CC tmp.attr_name, CC"address_locator");
	tmp.attr_type = UDI_ATTR_STRING;
	strcpy8(CC tmp.attr_value, CC"__kvchipset0");
	ret = vchipset->setEnumerationAttribute(&tmp);
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
	ret = chipset->setEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	strcpy8(CC tmp.attr_name, CC"address_locator");
	tmp.attr_type = UDI_ATTR_STRING;
	strcpy8(CC tmp.attr_value, CC"chipset0");
	ret = chipset->setEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	/* zbz_root enumeration attributes.
	 **/
	strcpy8(CC tmp.attr_name, CC"bus_type");
	tmp.attr_type = UDI_ATTR_STRING;
	strcpy8(CC tmp.attr_value, CC"zbz_root");
	ret = chipset->setEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	strcpy8(CC tmp.attr_name, CC"zbz_root_chipset_short_name");
	tmp.attr_type = UDI_ATTR_STRING;
	// strcpy8(CC tmp.attr_value, CC CHIPSET_SHORT_STRING);
	strcpy8(CC tmp.attr_value, CC"IBM-PC");
	ret = chipset->setEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	strcpy8(CC tmp.attr_name, CC"zbz_root_chipset_long_name");
	tmp.attr_type = UDI_ATTR_STRING;
	// strcpy8(CC tmp.attr_value, CC CHIPSET_LONG_STRING);
	strcpy8(CC tmp.attr_value, CC"IBM-PC-compatible-chipset");
	ret = chipset->setEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	strcpy8(CC tmp.attr_name, CC"zbz_root_chipset_using_smp");
	tmp.attr_type = UDI_ATTR_BOOLEAN;
	tmp.attr_value[0] = cpuTrib.usingChipsetSmpMode();
	ret = chipset->setEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	strcpy8(CC tmp.attr_name, CC"zbz_root_chipset_info0");
	tmp.attr_type = UDI_ATTR_STRING;
	tmp.attr_value[0] = '\0';
	ret = chipset->setEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	strcpy8(CC tmp.attr_name, CC"zbz_root_chipset_info1");
	tmp.attr_type = UDI_ATTR_STRING;
	tmp.attr_value[0] = '\0';
	ret = chipset->setEnumerationAttribute(&tmp);

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

