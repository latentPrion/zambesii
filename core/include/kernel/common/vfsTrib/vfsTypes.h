#ifndef _VIRTUAL_FILE_SYSTEM_TYPES_H
	#define _VIRTUAL_FILE_SYSTEM_TYPES_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/multipleReaderLock.h>
	#include <kernel/common/timerTrib/timeTypes.h>


#define VFSFILE_NAME_MAX_NCHARS		256
#define VFSDIR_NAME_MAX_NCHARS		256

class vfsDirC;
class vfsFileC;

class vfsDirInodeC
{
public:
	vfsDirInodeC(void);
	error_t initialize(void);
	~vfsDirInodeC(void);

public:
	ubit32			inodeLow/*, inodeHigh*/;
	sharedResourceGroupC<waitLockC, vfsDirC *>	subDirs;
	sharedResourceGroupC<waitLockC, vfsFileC *>	files;
	ubit32			nSubDirs;
	ubit32			nFiles;
	dateS			createdDate, modifiedDate, accessedDate;
	timeS			createdTime, modifiedTime, accessedTime;
};

class vfsFileInodeC
{
public:
	vfsFileInodeC(void);
	error_t initialize(void);
	~vfsFileInodeC(void);

public:
	ubit32			inodeLow/*, inodeHigh*/;
	// vfsCacheC		cache;
	// Max filesize supported by VFS depends on arch.
	uarch_t			fileSize;
};		


#define VFSFILE_FLAGS_OPEN	(1<<1)

class vfsFileC
{
public:
	vfsFileC(void);
	error_t initialize(void);
	~vfsFileC(void);

public:
	utf8Char		name[256];
	vfsDirC			*parent;
	vfsFileC		*next;
	vfsFileInodeC		*desc;
	ubit32			flags;
	ubit8			type;
};


#define VFSDIR_FLAGS_UNREAD	(1<<0)

class vfsDirC
{
public:
	vfsDirC(void);
	error_t initialize(void);
	~vfsDirC(void);

public:
	utf8Char		name[256];
	vfsDirC			*next, *parent;
	vfsDirInodeC		*desc;
	// fsDrvInstS		*fsDrv;
	ubit32			flags;
	ubit8			type;
};

#endif

