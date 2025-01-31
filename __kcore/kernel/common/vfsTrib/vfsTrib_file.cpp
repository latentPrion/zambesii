
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/vfsTrib/vfsTrib.h>


status_t VfsTrib::createFile(vfsDirC *dir, utf8Char *name, uarch_t)
{
	vfsFileC	*newFile;
	status_t	ret;

	// Allocate and initialize the new file descriptor.
	newFile = new vfsFileC;
	if (newFile == NULL) {
		return ERROR_MEMORY_NOMEM;
	};

	ret = newFile->initialize(name);
	if (ret != ERROR_SUCCESS) {
		return ret;
	};

	// Add the new file to the folder.
	dir->desc->addFileDesc(newFile);

	// Link the file descriptor to its parent folder.
	newFile->parent = dir;

	return ERROR_SUCCESS;
}

error_t VfsTrib::deleteFile(vfsDirINode *inode, utf8Char *name)
{
	return inode->removeFileDesc(name);
}

status_t VfsTrib::renameFile(vfsDirINode *, utf8Char *, utf8Char *)
{
	return ERROR_UNIMPLEMENTED;
}

