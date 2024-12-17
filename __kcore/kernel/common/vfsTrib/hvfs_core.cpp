
#include <kernel/common/vfsTrib/vfsTrib.h>
#include <kernel/common/vfsTrib/hierarchicalStorage.h>


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

error_t VfsTrib::registerDirInode(ubit32 inodeLow, hvfs::DirInode *inode)
{
	return dirInodeHash.insert(inodeLow, inode);
}
