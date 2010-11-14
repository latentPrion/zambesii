
#include <__kstdlib/__kclib/string16.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/vfsTrib/vfsTrib.h>
#include <kernel/common/vfsTrib/vfsTraverse.h>


error_t vfsTribC::createFolder(vfsDirC *dir, utf16Char *name, uarch_t)
{
	error_t		ret;
	vfsDirC		*newDir;

	// Make sure folder doesn't already exist.
	if (vfsTraverse::getDirDesc(dir->desc, name) != __KNULL) {
		return ERROR_INVALID_ARG_VAL;
	};

	newDir = new (dirDescCache->allocate()) vfsDirC;
	if (newDir == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};
	if ((ret = newDir->initialize()) != ERROR_SUCCESS) {
		return ret;
	};

	strcpy16(newDir->name, name);
	newDir->parent = dir;

	dir->desc->subDirs.lock.acquire();

	newDir->next = dir->desc->subDirs.rsrc;
	dir->desc->subDirs.rsrc = newDir;
	dir->desc->nSubDirs++;

	dir->desc->subDirs.lock.release();

	return ERROR_SUCCESS;
}

error_t vfsTribC::deleteFolder(vfsDirInodeC *inode, utf16Char *name)
{
	vfsDirC		*curDir, *prevDir;

	prevDir = __KNULL;

	inode->subDirs.lock.acquire();

	curDir = inode->subDirs.rsrc;
	for (; curDir != __KNULL; )
	{
		// If the folder exists:
		if (strcmp16(curDir->name, name) == 0)
		{
			if (prevDir != __KNULL) {
				prevDir->next = curDir->next;
			}
			else {
				inode->subDirs.rsrc = curDir->next;
			};
			inode->nSubDirs--;

			inode->subDirs.lock.release();

			curDir->~vfsDirC();
			dirDescCache->free(curDir);
			return ERROR_SUCCESS;
		};

		prevDir = curDir;
		curDir = curDir->next;
	};

	// Folder didn't exist.
	inode->subDirs.lock.release();
	return ERROR_INVALID_ARG_VAL;
}

