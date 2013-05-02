
#include <kernel/common/vfsTrib/hierarchicalStorage.h>


#define VFSTRIB_INODE_STACK_NITEMS	(4096)

// For now, VFS inodes are only 32-bits long.
multiLayerHashC<vfsDirInodeC>		dirInodeHash;
multiLayerHashC<vfsFileInodeC>		fileInodeHash;
// Stack of released VFS inode numbers.
arrayStackC<ubit32>	inodeStack;
// Object caches for the descriptors.

ubit32			inodeCounter;

// VFS Inode allocation.
error_t vfsTribC::getNewInode(ubit32 *inodeLow)
{
	// Try to pop an inode number off the stack.
	if (inodeStack.pop(inodeLow) == ERROR_SUCCESS) {
		return ERROR_SUCCESS;
	};

	// Else use the inode counter.
	if (inodeCounter == 0xFFFFFFFF) {
		return ERROR_CRITICAL;
	};

	*inodeLow = inodeCounter++;
	return ERROR_SUCCESS;
}

void vfsTribC::releaseDirInode(ubit32 inodeLow)
{
	inodeStack.push(inodeLow);
}

error_t registerDirInode(ubit32 inodeLow, vfsDirInodeC *inode)
{
	return dirInodeHash.insert(inodeLow, inode);
}

