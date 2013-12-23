
#include <chipset/chipset.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <kernel/common/zudiIndexServer.h>
#include <kernel/common/floodplainn/floodplainn.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/vfsTrib/vfsTrib.h>


fplainn::deviceC		byId(0, CC"by-id-tree"),
				byName(0, CC"by-name-tree"),
				byClass(0, CC"by-class-tree"),
				byPath(0, CC"by-path-tree");

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
	utf8Char *parentId, ubit16 childId, ubit32 /*flags*/,
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

	newDev = new fplainn::deviceC(childId, NULL);
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

syscallbackFuncF floodplainn_createRootDeviceReq1;
error_t floodplainnC::createRootDeviceReq(createRootDeviceReqCallF *callback)
{
	error_t				ret;
	fplainn::deviceC		*dev;
	udi_instance_attr_list_t	tmp;

	ret = createDevice(CC"by-id", 0, 0, &dev);
	if (ret != ERROR_SUCCESS) { return ret; };

	strcpy8(CC tmp.attr_name, CC"bus_type");
	tmp.attr_type = UDI_ATTR_STRING;
	strcpy8(CC tmp.attr_value, CC"zbz-root");
	ret = dev->setEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	strcpy8(CC tmp.attr_name, CC"rootbus_chipset_short_name");
	tmp.attr_type = UDI_ATTR_STRING;
	// strcpy8(CC tmp.attr_value, CC CHIPSET_SHORT_STRING);
	strcpy8(CC tmp.attr_value, CC"IBM-PC");
	ret = dev->setEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	strcpy8(CC tmp.attr_name, CC"rootbus_chipset_long_name");
	tmp.attr_type = UDI_ATTR_STRING;
	// strcpy8(CC tmp.attr_value, CC CHIPSET_LONG_STRING);
	strcpy8(CC tmp.attr_value, CC"IBM-PC-compatible-chipset");
	ret = dev->setEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	strcpy8(CC tmp.attr_name, CC"rootbus_chipset_using_smp");
	tmp.attr_type = UDI_ATTR_BOOLEAN;
	tmp.attr_value[0] = cpuTrib.usingChipsetSmpMode();
	ret = dev->setEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	strcpy8(CC tmp.attr_name, CC"rootbus_chipset_info0");
	tmp.attr_type = UDI_ATTR_STRING;
	tmp.attr_value[0] = '\0';
	ret = dev->setEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	strcpy8(CC tmp.attr_name, CC"rootbus_chipset_info1");
	tmp.attr_type = UDI_ATTR_STRING;
	tmp.attr_value[0] = '\0';
	ret = dev->setEnumerationAttribute(&tmp);

	zudiIndexServer::newDeviceInd(
		CC"by-id/0", zudiIndexServer::INDEX_KERNEL,
		newSyscallback(
			&floodplainn_createRootDeviceReq1, (voidF *)callback));

	return ret;
}

void floodplainn_createRootDeviceReq1(
	messageStreamC::iteratorS *gmsg, void (*callback)()
	)
{
	floodplainnC::createRootDeviceReqCallF	*_callback;
	floodplainnC::zudiIndexMsgS		*msg;
	error_t					ret=ERROR_SUCCESS;

	msg = (floodplainnC::zudiIndexMsgS *)gmsg;

	if (msg->header.error != ERROR_SUCCESS
		|| msg->info.action != zudiIndexServer::NDACTION_INSTANTIATE)
	{
		ret = ERROR_FATAL;
		printf(FATAL FPLAINN"createRootDevice: Failed to instantiate "
			"root device.\n");
	};

	_callback = (floodplainnC::createRootDeviceReqCallF *)callback;
	_callback(ret);
}

