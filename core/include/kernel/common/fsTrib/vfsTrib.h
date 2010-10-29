#ifndef _VIRTUAL_FILESYSTEM_TRIBUTARY_H
	#define _VIRTUAL_FILESYSTEM_TRIBUTARY_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/cachePool.h>
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
	vfsDirS *getDirectory(utf16Char *path, uarch_t flags);
	vfsDirS *createDirectory(
		vfsDirS *parent, utf16Char *dirName,
		uarch_t flags, status_t *status);

	void deleteDirectory(vfsDirS *parent, utf16Char *dirName);

	vfsFileS *getFile(utf16Char *path, uarch_t flags);
	vfsFileS *createFile(
		vfsDirS *parent, utf16Char *fileName,
		uarch_t flags, status_t *status);

	void deleteFile(vfsDirS *parent, utf16Char *fileName);

private:
	struct vfsTreeStateS
	{
		vfsDirS		*arr;
		ubit32		nTrees;
	};
	sharedResourceGroupC<multipleReaderLockC, vfsTreeStateS>	trees;
};

extern vfsTribC		vfsTrib;

#endif

