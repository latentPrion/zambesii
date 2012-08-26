#ifndef _VIRTUAL_FILE_SYSTEM_TYPES_H
	#define _VIRTUAL_FILE_SYSTEM_TYPES_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/multipleReaderLock.h>
	#include <kernel/common/timerTrib/timeTypes.h>


#define VFSDESC_TYPE_FILE		0x1
#define VFSDESC_TYPE_DIR		0x2

#define VFSFILE_NAME_MAX_NCHARS		255
#define VFSDIR_NAME_MAX_NCHARS		255

class vfsDirC;
class vfsFileC;

class vfsDirInodeC
{
public:
	vfsDirInodeC(ubit32 inodeHigh, ubit32 inodeLow);
	error_t initialize(void);
	~vfsDirInodeC(void);

	void dumpSubDirs(void);
	void dumpFiles(void);

public:
	void addDirDesc(vfsDirC *newDir);
	void addFileDesc(vfsFileC *newFile);
	status_t removeFileDesc(utf8Char *name);
	status_t removeDirDesc(utf8Char *name);

	vfsFileC *getFileDesc(utf8Char *name);
	vfsDirC *getDirDesc(utf8Char *name);

public:
	// These are the inode number on the CONCRETE fs, and not the VFS.
	ubit32			inodeLow, inodeHigh;
	sharedResourceGroupC<waitLockC, vfsDirC *>	subDirs;
	sharedResourceGroupC<waitLockC, vfsFileC *>	files;
	ubit32			nSubDirs;
	ubit32			nFiles;
	ubit32			refCount;
	date_t			createdDate, modifiedDate, accessedDate;
	time_t			createdTime, modifiedTime, accessedTime;
};

class vfsFileInodeC
{
public:
	vfsFileInodeC(ubit32 inodeHigh, ubit32 inodeLow, uarch_t fileSize);
	error_t initialize(void);
	~vfsFileInodeC(void);

public:
	// These are the inode number on the CONCRETE fs and not within the VFS.
	ubit32			inodeLow, inodeHigh;
	// vfsCacheC		cache;
	// fsDrvInstC		fsDrv;
	// Max filesize supported by VFS depends on arch.
	uarch_t			fileSize;
	ubit32			refCount;
	date_t			createdDate, modifiedDate, accessedDate;
	time_t			createdTime, modifiedTime, accessedTime;
};		


#define VFSFILE_FLAGS_OPEN	(1<<1)

class vfsFileC
{
public:
	vfsFileC(void);
	vfsFileC(vfsFileInodeC *inode);
	error_t initialize(
		utf8Char *name,
		ubit32 inodeHigh=0, ubit32 inodeLow=0, uarch_t fileSize=0);
	~vfsFileC(void);

public:
	status_t rename(utf8Char *newName);
public:
	utf8Char		*name;
	vfsDirC			*parent;
	vfsFileC		*next;
	vfsFileInodeC		*desc;
	ubit32			flags;
	ubit32			refCount;
	ubit8			type;
};


#define VFSDIR_FLAGS_UNREAD	(1<<0)

class vfsDirC
{
public:
	vfsDirC(void);
	vfsDirC(vfsDirInodeC *inode);
	error_t initialize(
		utf8Char *name, ubit32 inodeHigh=0, ubit32 inodeLow=0);

	~vfsDirC(void);

public:
	status_t rename(utf8Char *newName);
public:
	utf8Char		*name;
	vfsDirC			*next, *parent;
	vfsDirInodeC		*desc;
	// fsDrvInstS		*fsDrv;
	ubit32			flags;
	ubit32			refCount;
	ubit8			type;
};

#endif

