
#include <__kstdlib/__kclib/string16.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/vfsTrib/vfsTrib.h>


void vfsTribC::dumpTrees(void)
{
	vfsDirC		*curDir;

	__kprintf(NOTICE VFSTRIB"Dumping: %d trees.\n", trees->nSubDirs);

	// Iterate through all current trees and print debug info.
	trees->subDirs.lock.acquire();

	curDir = trees->subDirs.rsrc;
	for (uarch_t i=0; i<trees->nSubDirs && curDir != __KNULL;
		curDir = curDir->next, i++)
	{
		__kprintf(NOTICE VFSTRIB"Tree %d: inode: %u name: %[s].\n",
			i, curDir->desc->inodeLow, curDir->name);
	};

	trees->subDirs.lock.release();
}

vfsDirC *vfsTribC::getRootTree(void)
{
	vfsDirC		*ret;

	trees->subDirs.lock.acquire();

	// If there are no trees in the VFS:
	if (trees->subDirs.rsrc == __KNULL)
	{
		trees->subDirs.lock.release();
		return __KNULL;
	};

	// Get _vfs tree.
	ret = trees->subDirs.rsrc;

	trees->subDirs.lock.release();
	return ret;
}

error_t vfsTribC::createTree(utf16Char *name)
{
	vfsDirC		*dirDesc;
	error_t		ret;

	// Make sure that tree doesn't already exist.
	dirDesc = getDirDesc(trees, name);
	if (dirDesc != __KNULL) {
		return ERROR_SUCCESS;
	};

	ret = createFolder(&_vfs, name, 0);
	return ret;
}

error_t vfsTribC::deleteTree(utf16Char *name)
{
	return deleteFolder(trees, name);
}

error_t vfsTribC::setDefaultTree(utf16Char *name)
{
	vfsDirC		*curDesc, *prevDesc;

	prevDesc = __KNULL;

	trees->subDirs.lock.acquire();

	curDesc = trees->subDirs.rsrc;
	for (; curDesc != __KNULL; )
	{
		// If we've found the tree:
		if (strcmp16(curDesc->name, name) == 0)
		{
			// If the tree is already the default:
			if (prevDesc == __KNULL)
			{
				trees->subDirs.lock.release();
				return ERROR_SUCCESS;
			};

			prevDesc->next = curDesc->next;
			curDesc->next = trees->subDirs.rsrc;
			trees->subDirs.rsrc = curDesc;

			trees->subDirs.lock.release();
			return ERROR_SUCCESS;
		};

		prevDesc = curDesc;
		curDesc = curDesc->next;
	};

	trees->subDirs.lock.release();
	return ERROR_INVALID_ARG_VAL;
}

