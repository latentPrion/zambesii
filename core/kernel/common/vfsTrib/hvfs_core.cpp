
#include <kernel/common/vfsTrib/hierarchicalStorage.h>


#define VFSTRIB_INODE_STACK_NITEMS	(4096)

// For now, VFS inodes are only 32-bits long.
MultiLayerHash<vfsDirINode>		dirInodeHash;
MultiLayerHash<vfsFileINode>		fileInodeHash;
// Stack of released VFS inode numbers.
ArrayStack<ubit32>	inodeStack;
// Object caches for the descriptors.

ubit32			Inodeounter;

// VFS Inode allocation.
error_t VfsTrib::getNewInode(ubit32 *inodeLow)
{
	// Try to pop an inode number off the stack.
	if (inodeStack.pop(inodeLow) == ERROR_SUCCESS) {
		return ERROR_SUCCESS;
	};

	// Else use the inode counter.
	if (Inodeounter == 0xFFFFFFFF) {
		return ERROR_CRITICAL;
	};

	*inodeLow = Inodeounter++;
	return ERROR_SUCCESS;
}

void VfsTrib::releaseDirInode(ubit32 inodeLow)
{
	inodeStack.push(inodeLow);
}

error_t registerDirInode(ubit32 inodeLow, vfsDirINode *inode)
{
	return dirInodeHash.insert(inodeLow, inode);
}

