#ifndef _VIRTUAL_FILESYSTEM_TRIBUTARY_H
	#define _VIRTUAL_FILESYSTEM_TRIBUTARY_H

	#include <arch/arch.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/cachePool.h>
	#include <__kclasses/multiLayerHash.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>
	#include <kernel/common/vfsTrib/vfsTypes.h>

#define VFSTRIB			"VFS Trib: "

class vfsTribC
:
public tributaryC
{
public:
	vfsTribC(void);
	error_t initialize(void) { return ERROR_SUCCESS; };
	~vfsTribC(void);

	void dumpTrees(void);

public:
	vfsDirC *getDirectory(utf16Char *path, uarch_t flags);
	vfsDirC *createDirectory(
		vfsDirC *parent, utf16Char *dirName,
		uarch_t flags, status_t *status);

	void deleteDirectory(vfsDirC *parent, utf16Char *dirName);

	vfsFileC *getFile(utf16Char *path, uarch_t flags);
	vfsFileC *createFile(
		vfsDirC *parent, utf16Char *fileName,
		uarch_t flags, status_t *status);

	void deleteFile(vfsDirC *parent, utf16Char *fileName);

	vfsDirC *createTree(utf16Char *name, uarch_t flags);
	error_t deleteTree(utf16Char *name);
	error_t setDefaultTree(utf16Char *name);

private:
	struct vfsTreeStateS
	{
		vfsDirC		*arr;
		ubit32		nTrees;
	};
	sharedResourceGroupC<waitLockC, vfsTreeStateS>	trees;

	/* Supports 32-bit and 64-bit VFS inodes, even on a 32-bit build.
	 * Can be easily extended to support 128-bit, and upwards with changes
	 * only to the following two lines.
	 **/
	multiLayerHashC< multiLayerHashC<vfsDirInodeC> >	dirDescHash;
	multiLayerHashC< multiLayerHashC<vfsFileInodeC> >	fileDescHash;
};

extern vfsTribC		vfsTrib;

#endif

