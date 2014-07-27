#ifndef _HIERARCHICAL_STORAGE_CURRENTT_H
	#define _HIERARCHICAL_STORAGE_CURRENTT_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kclib/string8.h>
	#include <kernel/common/vfsTrib/vfsTypes.h>

#define HVFS_TAG_NAME_MAXLEN		(256)
#define HVFS_PATH_MAXLEN		(256)

namespace hvfs
{
	/**	InodeId:
	 * Represents a concrete inode ID on a concrete filesystem instance.
	 **********************************************************************/
	class InodeId
	{
	public:
		InodeId(uarch_t high, uarch_t low)
		:
		high(high), low(low)
		{}

		~InodeId(void) {}

	private:
		uarch_t		high, low;
	};

	/**	Tag typedefs.
	 **********************************************************************/
	typedef vfs::Tag<HVFS_TAG_NAME_MAXLEN>		Tag;

	/**	Inode:
	 * Basic class with the common-denominator inode members: dates and
	 * sizes, and the concrete inode ID.
	 **********************************************************************/
	class Inode
	{
	public:
		class FileSize;

		Inode(
			InodeId concreteId, FileSize size,
			sTimestamp createdTime, sTimestamp modifiedTime,
			sTimestamp accessedTime)
		:
		concreteId(concreteId), size(size),
		createdTime(createdTime), modifiedTime(modifiedTime),
		accessedTime(accessedTime)
		{}

		error_t initialize(void) { return ERROR_SUCCESS; }
		~Inode(void) {}

	public:
		/**	Inode::FileSize:
		 * Basic abstraction of a file size. Uses uarch_t so it can
		 * automagically scale based on target architecture.
		 **************************************************************/
		class FileSize
		{
		public:
			FileSize(uarch_t high, uarch_t med, uarch_t low)
			:
			low(low), med(med), high(high)
			{}

			// Doesn't need an initialize().
			// error_t initialize(void) {}
			FileSize(void) {}

		private:
			uarch_t		low, med, high;
		};

		FileSize getSize(void) { return size; }
		InodeId getConcreteId(void) { return concreteId; }
		sTimestamp getCreatedTime(void) { return createdTime; }
		sTimestamp getModifiedTime(void) { return modifiedTime; }
		sTimestamp getAccessedTime(void) { return accessedTime; }

	private:
		InodeId		concreteId;
		FileSize		size;
		sTimestamp		createdTime, modifiedTime, accessedTime;
	};

	/**	FileInode:
	 * Abstraction of a file inode for the storage VFS.
	 **********************************************************************/
	class FileInode
	:
	public vfs::Inode, public Inode
	{
	public:
		FileInode(
			InodeId concreteId, FileSize size,
			sTimestamp createdTime, sTimestamp modifiedTime,
			sTimestamp accessedTime)
		:
		hvfs::Inode(
			concreteId, size,
			createdTime, modifiedTime, accessedTime)
		{}

		error_t initialize(void)
		{
			error_t		ret;

			ret = vfs::Inode::initialize();
			if (ret != ERROR_SUCCESS) { return ret; }
			return hvfs::Inode::initialize();
		}

		~FileInode(void) {}
	};

	/**	SymlinkInode:
	 * Abstraction of a symlink inode. Basically a FileInode with a string
	 * inside of it.
	 **********************************************************************/
	class SymlinkInode
	:
	public FileInode
	{
	public:
		SymlinkInode(
			InodeId concreteId, FileSize size,
			sTimestamp createdTime, sTimestamp modifiedTime,
			sTimestamp accessedTime)
		:
		FileInode(
			concreteId, size,
			createdTime, modifiedTime, accessedTime)
		{
			strncpy8(
				this->fullName, fullName,
				symlinkFullNameMaxLength);

			this->fullName[symlinkFullNameMaxLength - 1] = '\0';
		}

		error_t initialize(void) { return FileInode::initialize(); }
		~SymlinkInode(void) {}

	public:
		const utf8Char *getFullName(void) { return fullName; }

	private:
		static const ubit16	symlinkFullNameMaxLength=512;
		utf8Char		fullName[symlinkFullNameMaxLength];
	};

	/**	DirInode:
	 * Storage VFS layer abstraction of a DirInode with creation date
	 * size, etc. attached.
	 **********************************************************************/
	class DirInode
	:
	public vfs::DirInode<Tag>, public hvfs::Inode
	{
	public:
		DirInode(
			InodeId concreteId, FileSize size,
			sTimestamp createdTime, sTimestamp modifiedTime,
			sTimestamp accessedTime)
		:
		hvfs::Inode(
			concreteId, size,
			createdTime, modifiedTime, accessedTime)
		{}

		error_t initialize(void)
			{ return vfs::DirInode<Tag>::initialize(); }

		~DirInode(void) {}

	private:
	};

	/**	Currentt:
	 * HVFS Currentt type.
	 **********************************************************************/
	class Currentt
	:
	public vfs::Currentt
	{
	public:
		Currentt(void)
		:
		vfs::Currentt(static_cast<utf8Char>( 'h' )),
		rootTag(
			CC"Zambesii Hierarchical VFS Currentt", vfs::DIR,
			&rootTag, &rootInode),

		rootInode(
			InodeId(0, 0), hvfs::Inode::FileSize(0, 0, 0),
			sTimestamp(0, 0, 0),
			sTimestamp(0, 0, 0),
			sTimestamp(0, 0, 0))
		{}

		error_t initialize(void)
			{ return vfs::Currentt::initialize(); }

		~Currentt(void) {}

	public:
		Tag *getRoot(void) { return &rootTag; }
		/*// VFS path traversal.
		status_t getPath(utf8Char *path, ubit8 *type, void **ret);

		// Folder manipulation.
		error_t createFolder(vfsDirC *dir, utf8Char *name, uarch_t flags=0);
		error_t deleteFolder(vfsDirINode *inode, utf8Char *name);
		status_t renameFolder(
			vfsDirINode *inode, utf8Char *oldName, utf8Char *newName);

		// File manipulation.
		error_t createFile(vfsDirC *dir, utf8Char *name, uarch_t flags=0);
		error_t deleteFile(vfsDirINode *inode, utf8Char *name);
		status_t renameFile(
			vfsDirINode *inode, utf8Char *oldName, utf8Char *newName);

		// Tree manipulation.
		vfsDirC *getDefaultTree(void);
		vfsDirC *getTree(utf8Char *name);
		error_t createTree(utf8Char *name, uarch_t flags=0);
		error_t deleteTree(utf8Char *name);
		error_t setDefaultTree(utf8Char *name);*/

	private:
		struct sDefaultTreeInfo
		{
			sDefaultTreeInfo(void)
			:
			tag(NULL)
			{
				name[0] = '\0';
			}

			utf8Char	name[HVFS_TAG_NAME_MAXLEN];
			Tag		*tag;
		};

		class SlamCache	*Tagache;
		Tag			rootTag;
		DirInode		rootInode;
		SharedResourceGroup<MultipleReaderLock, sDefaultTreeInfo>
			defaultTree;
	};
}

#endif

