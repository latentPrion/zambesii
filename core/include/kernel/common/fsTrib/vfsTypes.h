#ifndef _VIRTUAL_FILE_SYSTEM_TYPES_H
	#define _VIRTUAL_FILE_SYSTEM_TYPES_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/timerTrib/timeTypes.h>


struct vfsDirC;
struct vfsFileC;

struct vfsDirDescC
{
	ubit32			inodeLow, inodeHigh;
	sharedResourceGroupC<multipleReaderLockC, vfsDirC *>	subDirs;
	sharedResourceGroupC<multipleReaderLockC, vfsFileC *>	files;
	ubit32			nSubdirs;
	ubit32			nFiles;
	dateS			createdDate, modifiedDate, accessedDate;
	timeS			createdTime, modifiedTime, accessedTime;
};

struct vfsFileDescC
{
	ubit32			inodeLow, inodeHigh;
	vfsCacheC		cache;
	// Max filesize supported by VFS depends on arch.
	uarch_t			fileSize;
};		


#define VFSFILE_FLAGS_OPEN	(1<<1)

struct vfsFileC
{
	ubit8			type;
	utf16Char		name[128];
	vfsFileDescC		*desc;
	ubit32			flags;
};


#define VFSDIR_FLAGS_UNREAD	(1<<0)

struct vfsDirC
{
	ubit8			type;
	utf16Char		name[128];
	vfsDirDescC		*desc;
	// fsDrvInstS		*fsDrv;
	ubit32			flags;
};

#endif

