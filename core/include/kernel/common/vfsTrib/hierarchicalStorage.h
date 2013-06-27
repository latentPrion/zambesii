#ifndef _HIERARCHICAL_STORAGE_CURRENTT_H
	#define _HIERARCHICAL_STORAGE_CURRENTT_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kclib/string8.h>
	#include <kernel/common/vfsTrib/vfsTypes.h>

#define HVFS_TAG_NAME_MAXLEN	(256)

namespace hvfs
{
	/**	inodeIdC:
	 * Represents a concrete inode ID on a concrete filesystem instance.
	 **********************************************************************/
	class inodeIdC
	{
	public:
		inodeIdC(uarch_t high, uarch_t low)
		:
		high(high), low(low)
		{}

		~inodeIdC(void) {}

	private:
		uarch_t		high, low;
	};

	/**	Tag typedefs.
	 **********************************************************************/
	typedef vfs::tagC<HVFS_TAG_NAME_MAXLEN>		tagC;

	/**	inodeC:
	 * Basic class with the common-denominator inode members: dates and
	 * sizes, and the concrete inode ID.
	 **********************************************************************/
	class inodeC
	{
	public:
		class fileSizeC;

		inodeC(
			inodeIdC concreteId, fileSizeC size,
			timestampS createdTime, timestampS modifiedTime,
			timestampS accessedTime)
		:
		concreteId(concreteId), size(size),
		createdTime(createdTime), modifiedTime(modifiedTime),
		accessedTime(accessedTime)
		{}

		error_t initialize(void) { return ERROR_SUCCESS; }
		~inodeC(void) {}

	public:
		/**	inodeC::fileSizeC:
		 * Basic abstraction of a file size. Uses uarch_t so it can
		 * automagically scale based on target architecture.
		 **************************************************************/
		class fileSizeC
		{
		public:
			fileSizeC(uarch_t high, uarch_t med, uarch_t low)
			:
			low(low), med(med), high(high)
			{}

			// Doesn't need an initialize().
			// error_t initialize(void) {}
			fileSizeC(void) {}

		private:
			uarch_t		low, med, high;
		};

		fileSizeC getSize(void) { return size; }
		inodeIdC getConcreteId(void) { return concreteId; }
		timestampS getCreatedTime(void) { return createdTime; }
		timestampS getModifiedTime(void) { return modifiedTime; }
		timestampS getAccessedTime(void) { return accessedTime; }

	private:
		inodeIdC		concreteId;
		fileSizeC		size;
		timestampS		createdTime, modifiedTime, accessedTime;
	};

	/**	fileInodeC:
	 * Abstraction of a file inode for the storage VFS.
	 **********************************************************************/
	class fileInodeC
	:
	public vfs::inodeC, public inodeC
	{
	public:
		fileInodeC(
			inodeIdC concreteId, fileSizeC size,
			timestampS createdTime, timestampS modifiedTime,
			timestampS accessedTime)
		:
		hvfs::inodeC(
			concreteId, size,
			createdTime, modifiedTime, accessedTime)
		{}

		error_t initialize(void)
		{
			error_t		ret;

			ret = vfs::inodeC::initialize();
			if (ret != ERROR_SUCCESS) { return ret; }
			return inodeC::initialize();
		}

		~fileInodeC(void) {}
	};

	/**	symlinkInodeC:
	 * Abstraction of a symlink inode. Basically a fileInodeC with a string
	 * inside of it.
	 **********************************************************************/
	class symlinkInodeC
	:
	public fileInodeC
	{
	public:
		symlinkInodeC(
			inodeIdC concreteId, fileSizeC size,
			timestampS createdTime, timestampS modifiedTime,
			timestampS accessedTime)
		:
		fileInodeC(
			concreteId, size,
			createdTime, modifiedTime, accessedTime)
		{
			strncpy8(
				this->fullName, fullName,
				symlinkFullNameMaxLength);

			this->fullName[symlinkFullNameMaxLength - 1] = '\0';
		}

		error_t initialize(void) { return fileInodeC::initialize(); }
		~symlinkInodeC(void) {}

	public:
		const utf8Char *getFullName(void) { return fullName; }

	private:
		static const ubit16	symlinkFullNameMaxLength=512;
		utf8Char		fullName[symlinkFullNameMaxLength];
	};

	/**	dirInodeC:
	 * Storage VFS layer abstraction of a dirInodeC with creation date
	 * size, etc. attached.
	 **********************************************************************/
	class dirInodeC
	:
	public vfs::dirInodeC<tagC>, public hvfs::inodeC
	{
	public:
		dirInodeC(
			inodeIdC concreteId, fileSizeC size,
			timestampS createdTime, timestampS modifiedTime,
			timestampS accessedTime)
		:
		hvfs::inodeC(
			concreteId, size,
			createdTime, modifiedTime, accessedTime)
		{}

		error_t initialize(void)
			{ return vfs::dirInodeC<tagC>::initialize(); }

		~dirInodeC(void) {}

	private:
	};

	/**	currenttC:
	 * HVFS Currentt type.
	 **********************************************************************/
	class currenttC
	:
	public vfs::currenttC
	{
	public:
		currenttC(void)
		:
		vfs::currenttC(static_cast<utf8Char>( 'h' )),
		rootTag(
			CC"Zambesii Hierarchical VFS Currentt", vfs::DIR, 
			&rootTag, &rootInode),

		rootInode(
			inodeIdC(0, 0), hvfs::inodeC::fileSizeC(0, 0, 0),
			timestampS(0, 0, 0),
			timestampS(0, 0, 0),
			timestampS(0, 0, 0))
		{}

		error_t initialize(void)
			{ return vfs::currenttC::initialize(); }

		~currenttC(void) {}

	public:
		tagC *getRoot(void) { return &rootTag; }
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

			utf8Char	name[HVFS_TAG_NAME_MAXLEN];
			tagC		*tag;
		};

		class slamCacheC	*tagCache;
		tagC			rootTag;
		dirInodeC		rootInode;			
		sharedResourceGroupC<multipleReaderLockC, defaultTreeInfoS>
			defaultTree;
	};
}

#endif

