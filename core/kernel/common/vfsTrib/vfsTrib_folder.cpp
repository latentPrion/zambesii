
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <kernel/common/vfsTrib/vfsTrib.h>
#include <kernel/common/vfsTrib/vfsTraverse.h>


status_t vfsTribC::createFolder(vfsDirC *dir, utf8Char *name, uarch_t)
{
	status_t	ret;
	vfsDirC		*newDir;

	ret = vfsTraverse::validateSegment(name);
	if (ret != ERROR_SUCCESS) {
		return ret;
	};
	// Make sure folder/file with same name doesn't already exist.
	if (dir->desc->getDirDesc(name) != NULL
		|| dir->desc->getFileDesc(name) != NULL)
	{
		return ERROR_INVALID_ARG_VAL;
	};

	newDir = new (dirDescCache->allocate()) vfsDirC;
	if (newDir == NULL) {
		return ERROR_MEMORY_NOMEM;
	};
	if ((ret = newDir->initialize(name)) != ERROR_SUCCESS)
	{
		newDir->~vfsDirC();
		dirDescCache->free(newDir);
		return ret;
	};

	newDir->parent = dir;
	dir->desc->addDirDesc(newDir);

	return ERROR_SUCCESS;
}

status_t vfsTribC::deleteFolder(vfsDirInodeC *inode, utf8Char *name)
{
	return inode->removeDirDesc(name);
}

