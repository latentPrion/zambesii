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
	#include <kernel/common/multipleReaderLock.h>
	#include <kernel/common/vfsTrib/vfsTypes.h>

#define VFSTRIB			"VFS Trib: "

#define VFSTRIB_INODE_STACK_NITEMS	(4096)

#define VFSPATH_TYPE_DIR		0x1
#define VFSPATH_TYPE_FILE		0x2

#define VFSPATH_INVALID			15
#define VFSPATH_INVALID_CHAR		16

class vfsTribC
:
public tributaryC
{
friend class vfsDirInodeC;
public:
	vfsTribC(void);
	error_t initialize(void);
	~vfsTribC(void);

	void dumpTrees(void);

public:
	// VFS path traversal.
	status_t getPath(utf8Char *path, ubit8 *type, void **ret);

	// Folder manipulation.
	error_t createFolder(vfsDirC *dir, utf8Char *name, uarch_t flags=0);
	error_t deleteFolder(vfsDirInodeC *inode, utf8Char *name);
	status_t renameFolder(
		vfsDirInodeC *inode, utf8Char *oldName, utf8Char *newName);

	// File manipulation.
	error_t createFile(vfsDirC *dir, utf8Char *name, uarch_t flags=0);
	error_t deleteFile(vfsDirInodeC *inode, utf8Char *name);
	status_t renameFile(
		vfsDirInodeC *inode, utf8Char *oldName, utf8Char *newName);

	// Tree manipulation.
	vfsDirC *getDefaultTree(void);
	vfsDirC *getTree(utf8Char *name);
	error_t createTree(utf8Char *name, uarch_t flags=0);
	error_t deleteTree(utf8Char *name);
	error_t setDefaultTree(utf8Char *name);

public:
	// VFS Inode allocation.
	error_t getNewInode(ubit32 *inodeLow);
	void releaseDirInode(ubit32 inodeLow);
	error_t registerDirInode(ubit32 inodeLow, vfsDirInodeC *inode) {
		return dirInodeHash.insert(inodeLow, inode);
	};

public:
	// For now, VFS inodes are only 32-bits long.
	multiLayerHashC<vfsDirInodeC>		dirInodeHash;
	multiLayerHashC<vfsFileInodeC>		fileInodeHash;
	// Stack of released VFS inode numbers.
	arrayStackC<ubit32>	inodeStack;
	// Object caches for the descriptors.
	slamCacheC		*fileDescCache;
	slamCacheC		*dirDescCache;

	ubit32			inodeCounter;

	// Default VFS tree for unix styled root.
	sharedResourceGroupC<multipleReaderLockC, vfsDirC *>	defaultTree;

	// The actual VFS directory hierarchy.
	vfsDirC			_vfs;
	vfsDirInodeC		*trees;
};

extern vfsTribC		vfsTrib;

#endif

