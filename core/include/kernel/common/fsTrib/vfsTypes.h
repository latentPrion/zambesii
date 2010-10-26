#ifndef _VIRTUAL_FILE_SYSTEM_TYPES_H
	#define _VIRTUAL_FILE_SYSTEM_TYPES_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/timerTrib/timeTypes.h>


struct vfsDirS;
struct vfsFileS;

struct vfsDirDescS
{
	ubit32			inodeLow, inodeHigh;
	sharedResourceGroupC<multipleReaderLockC, vfsDirS *>	subDirs;
	sharedResourceGroupC<multipleReaderLockC, vfsFileS *> files;
	ubit32			nSubdirs;
	ubit32			nFiles;
	dateS			createdDate, modifiedDate, accessedDate;
	timeS			createdTime, modifiedTime, accessedTime;
};

struct vfsFileDescS
{
	ubit32			inodeLow, inodeHigh;
	vfsCacheC		cache;
	// Max filesize supported by VFS depends on arch.
	uarch_t			fileSize;
};		


#define VFSFILE_FLAGS_OPEN	(1<<1)

struct vfsFileS
{
	utf16Char		name[256];
	vfsFileDescS		desc;
	ubit32			flags;
	ubit8			type;
};


#define VFSDIR_FLAGS_UNREAD	(1<<0)

struct vfsDirS
{
	utf16Char		name[256];
	vfsDirDescS		desc;
	// fsDrvInstS		*fsDrv;
	ubit32			flags;
};

#endif

