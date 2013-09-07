
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <kernel/common/floodplainn/floodplainn.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/vfsTrib/vfsTrib.h>


fplainn::deviceC		byId(0, CC"by-id-tree"),
				byName(0, CC"by-name-tree"),
				byClass(0, CC"by-class-tree"),
				byPath(0, CC"by-path-tree");

error_t floodplainnC::initialize(void)
{
	fvfs::tagC		*root, *tmp;
	error_t			ret;

	/**	EXPLANATION:
	 * Create the four sub-trees and then exit.
	 **/
	root = vfsTrib.getFvfs()->getRoot();

	ret = root->createChild(CC"by-id", &byId, &tmp);
	if (ret != ERROR_SUCCESS) { return ret; };
	ret = root->createChild(CC"by-name", &byName, &tmp);
	if (ret != ERROR_SUCCESS) { return ret; };
	ret = root->createChild(CC"by-class", &byClass, &tmp);
	if (ret != ERROR_SUCCESS) { return ret; };
	return root->createChild(CC"by-path", &byPath, &tmp);
}

void floodplainnC::__kdriverEntry(void)
{
	threadC		*self;

	self = (threadC *)cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentTask();

	__kprintf(NOTICE"!!!Kernel driver running. About to dormant.\n");
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
	if (parentId == __KNULL || device == __KNULL)
		{ return ERROR_INVALID_ARG; };

	if (!isByIdPath(parentId)) { return ERROR_INVALID_ARG_VAL; };

	// Get the parent node's VFS tag.
	ret = vfsTrib.getFvfs()->getPath(parentId, &parentTag);
	if (ret != ERROR_SUCCESS) { return ret; };

	newDev = new fplainn::deviceC(childId, __KNULL);
	if (newDev == __KNULL) { return ERROR_MEMORY_NOMEM; };

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

	if (parentId == __KNULL) { return ERROR_INVALID_ARG; };

	if (!isByIdPath(parentId)) { return ERROR_INVALID_ARG_VAL; };
	ret = vfsTrib.getFvfs()->getPath(parentId, &parentTag);
	if (ret != ERROR_SUCCESS) { return ret; };

	utf8Char	tmpBuff[FVFS_TAG_NAME_MAXLEN];

	// Get the child tag.
	snprintf(tmpBuff, FVFS_TAG_NAME_MAXLEN, CC"%d", childId);
	childTag = parentTag->getChild(tmpBuff);
	if (childTag == __KNULL) { return ERROR_NOT_FOUND; };

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

error_t floodplainnC::loadDriver(
	utf8Char *devicePath, indexLevelE indexLevel, ubit32 /*flags*/
	)
{
	driverIndexRequestS		*request;
	taskC				*currTask;

	currTask = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask();

	// Allocate and queue the new request.
	request = new driverIndexRequestS(devicePath, indexLevel);
	if (request == __KNULL) { return ERROR_MEMORY_NOMEM; };

	if (currTask->getType() == task::PER_CPU)
	{
		request->header.sourceId = request->header.targetId =
			cpuTrib.getCurrentCpuStream()->cpuId;
	}
	else
	{
		request->header.sourceId = request->header.targetId =
			((threadC *)currTask)->getFullId();
	};
		
	request->header.privateData = __KNULL;
	request->header.flags = 0;
	request->header.size = sizeof(*request);
	request->header.subsystem = ZMESSAGE_SUBSYSTEM_USER0;
	request->header.function = ZMESSAGE_FPLAINN_LOADDRIVER;

	return messageStreamC::enqueueOnThread(
		indexerThreadId, &request->header);
}

