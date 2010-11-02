#ifndef _VIRTUAL_FILESYSTEM_TRIBUTARY_H
	#define _VIRTUAL_FILESYSTEM_TRIBUTARY_H

	#include <arch/arch.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/cachePool.h>
	#include <__kclasses/arrayStack.h>
	#include <__kclasses/multiLayerHash.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>
	#include <kernel/common/vfsTrib/vfsTypes.h>

#define VFSTRIB			"VFS Trib: "

#define VFSTRIB_INODE_STACK_NITEMS	(4096)

class vfsTribC
:
public tributaryC
{
public:
	vfsTribC(void);
	error_t initialize(void);
	~vfsTribC(void);

	void dumpTrees(void);

public:
	// VFS Inode allocation.
	error_t getNewInode(ubit32 *inodeLow);
	void releaseDirInode(ubit32 inodeLow);
	error_t registerDirInode(ubit32 inodeLow, vfsDirInodeC *inode) {
		return dirInodeHash.insert(inodeLow, inode);
	};

	// Folder manipulation.
	error_t createFolder(vfsDirC *dir, utf16Char *name, uarch_t flags=0);
	error_t deleteFolder(vfsDirInodeC *inode, utf16Char *name);

	// Tree manipulation.
	vfsDirC *getRootTree(void);
	error_t createTree(utf16Char *name);
	error_t deleteTree(utf16Char *name);
	error_t setDefaultTree(utf16Char *name);

private:
	vfsFileC *getFileDesc(vfsDirInodeC *inode, utf16Char *name);
	vfsDirC *getDirDesc(vfsDirInodeC *inode, utf16Char *name);

private:
	// For now, VFS inodes are only 32-bits long.
	multiLayerHashC<vfsDirInodeC>		dirInodeHash;
	multiLayerHashC<vfsFileInodeC>		fileInodeHash;
	// Stack of released VFS inode numbers.
	arrayStackC<ubit32>	inodeStack;
	// Object caches for the descriptors.
	slamCacheC		*fileDescCache;
	slamCacheC		*dirDescCache;

	ubit32			inodeCounter;

	// The actual VFS directory hierarchy.
	vfsDirC			_vfs;
	vfsDirInodeC		*trees;
};

extern vfsTribC		vfsTrib;

#endif

