#ifndef _HIERARCHICAL_STORAGE_CURRENTT_H
	#define _HIERARCHICAL_STORAGE_CURRENTT_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/vfsTrib/vfsTypes.h>

#define HVFS_TAG_NAME_MAX_NCHARS	(256)

namespace hvfs
{
	class inodeC
	{
	public:
		inodeC(uarch_t high, uarch_t low)
		:
		high(high), low(low)
		{}

		~inodeC(void) {}

	private:
		uarch_t		high, low;
	};

	class dirInodeC
	:
	public vfs::dirInodeC<HVFS_TAG_NAME_MAX_NCHARS>
	{
	public:
		dirInodeC(inodeC concreteId)
		:
		concreteInodeId(concreteId)
		{}

		error_t initialize(void)
		{
			return vfs::dirInodeC<HVFS_TAG_NAME_MAX_NCHARS>
				::initialize();
		}

		~dirInodeC(void) {}

	private:
		inodeC		concreteInodeId;
		timestampS	createdTime, modifiedTime, accessedTime;
	};

	class fileInodeC
	:
	public vfs::leafInodeC
	{
	public:
		fileInodeC(inodeC concreteId)
		:
		concreteInodeId(concreteId)
		{}

		error_t initialize(void)
			{ return vfs::leafInodeC::initialize(); }

		~fileInodeC(void) {}

	public:
		void setSize(uarch_t high, uarch_t mid, uarch_t low)
			{ sizeHigh = high; sizeMid = mid; sizeLow = low; }

	private:
		inodeC		concreteInodeId;
		timestampS	createdTime, modifiedTime, accessedTime;
		uarch_t		sizeHigh, sizeMid, sizeLow;
	};

	typedef vfs::tagC<dirInodeC, HVFS_TAG_NAME_MAX_NCHARS>	dirTagC;
	typedef vfs::tagC<fileInodeC, HVFS_TAG_NAME_MAX_NCHARS>	fileTagC;

	class currenttC
	:
	public vfs::currenttC
	{
	public:
		currenttC(utf8Char prefix)
		:
		vfs::currenttC(prefix),
		rootTag(
			CC"Zambesii Hierarchical VFS Currentt",
			&rootTag, &rootInode),

		rootInode(inodeC(0, 0))
		{}

		error_t initialize(void)
			{ return vfs::currenttC::initialize(); }

		~currenttC(void) {}

	public:
		/*// VFS path traversal.
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
		error_t setDefaultTree(utf8Char *name);*/

	private:
		struct defaultTreeInfoS
		{
			defaultTreeInfoS(void)
			:
			tag(__KNULL)
			{
				name[0] = '\0';
			}

			utf8Char	name[HVFS_TAG_NAME_MAX_NCHARS];
			hvfs::dirTagC	*tag;
		};

		class slamCacheC	*tagCache;
		dirTagC			rootTag;
		dirInodeC		rootInode;			
		sharedResourceGroupC<multipleReaderLockC, defaultTreeInfoS>
			defaultTree;
	};
}

#endif

