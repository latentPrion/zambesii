
#include <__kstdlib/__kclib/string8.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/vfsTrib/vfsTrib.h>
#include <kernel/common/vfsTrib/vfsTraverse.h>


void vfsTribC::dumpTrees(void)
{
	vfsDirC		*curDir;

	__kprintf(NOTICE VFSTRIB"Dumping: %d trees.\n", trees->nSubDirs);
	__kprintf(NOTICE VFSTRIB"\tDefault tree: inode: %u, :%s.\n",
		getDefaultTree()->desc->inodeLow, getDefaultTree()->name);

	// Iterate through all current trees and print debug info.
	trees->subDirs.lock.acquire();

	curDir = trees->subDirs.rsrc;
	for (uarch_t i=0; i<trees->nSubDirs && curDir != NULL;
		curDir = curDir->next, i++)
	{
		__kprintf(NOTICE"\tTree %d: inode: %u name: :%s.\n",
			i, curDir->desc->inodeLow, curDir->name);
	};

	trees->subDirs.lock.release();
}

vfsDirC *vfsTribC::getDefaultTree(void)
{
	vfsDirC		*ret;
	uarch_t		rwFlags;

	defaultTree.lock.readAcquire(&rwFlags);
	ret = defaultTree.rsrc;
	defaultTree.lock.readRelease(rwFlags);

	return ret;
}

vfsDirC *vfsTribC::getTree(utf8Char *name)
{
	if (name[0] == ':') {
		name++;
	};
	return trees->getDirDesc(name);
}

error_t vfsTribC::createTree(utf8Char *name, uarch_t)
{
	vfsDirC		*dirDesc;
	error_t		ret;

	if (name[0] == ':') {
		name++;
	};

	// Make sure that tree doesn't already exist.
	dirDesc = trees->getDirDesc(name);
	if (dirDesc != NULL) {
		return ERROR_SUCCESS;
	};

	ret = createFolder(&_vfs, name, 0);
	return ret;
}

error_t vfsTribC::deleteTree(utf8Char *name)
{
	vfsDirC		*curDef, *cur;
	ubit8		foundNewDefault=0;

	if (name[0] == ':') {
		name++;
	};

	// Check to see if it's the default. If it is, set the next in line.
	curDef = getDefaultTree();
	if (trees->getDirDesc(name) == curDef)
	{
		trees->subDirs.lock.acquire();

		cur = trees->subDirs.rsrc;
		for (; cur != NULL; cur = cur->next)
		{
			if (cur == curDef) {
				continue;
			};

			// Set next in line as default and move on.
			defaultTree.lock.writeAcquire();
			defaultTree.rsrc = cur;
			defaultTree.lock.writeRelease();

			foundNewDefault = 1;
			break;
		};

		trees->subDirs.lock.release();
	}
	else
	{
		/* If we weren't deleting the default, set this to 1 so the
		 * code below to actually remove the tree will be executed.
		 **/
		foundNewDefault = 1;
	};

	if (foundNewDefault) {
		return deleteFolder(trees, name);
	};
	return ERROR_CRITICAL;
}

error_t vfsTribC::setDefaultTree(utf8Char *name)
{
	vfsDirC		*dir;

	if (name[0] == ':') {
		name++;
	};

	dir = trees->getDirDesc(name);
	if (dir == NULL) {
		return ERROR_INVALID_ARG_VAL;
	};

	defaultTree.lock.writeAcquire();
	defaultTree.rsrc = dir;
	defaultTree.lock.writeRelease();

	return ERROR_SUCCESS;
}

