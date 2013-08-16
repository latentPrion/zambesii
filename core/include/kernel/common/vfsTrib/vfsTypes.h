#ifndef _VIRTUAL_FILE_SYSTEM_TYPES_H
	#define _VIRTUAL_FILE_SYSTEM_TYPES_H

	#include <arch/cpuControl.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/ptrList.h>
	#include <kernel/common/timerTrib/timeTypes.h>


#define VFSDESC_TYPE_FILE		0x1
#define VFSDESC_TYPE_DIR		0x2

namespace vfs
{
	enum tagTypeE { DIR=1, MOUNTPOINT, FILE, SYMLINK, DEVICE };

	/* Returns:
	 *	ERROR_NOT_FOUND: if no instance of 'splitChar' is found before
	 *		the first instance of '\0'.
	 *	ERROR_INVALID_RESOURCE_NAME: If no instance of 'splitChar' is
	 *		found before 'maxLength' characters have been searched.
	 *	A signed integer if 'splitChar' is found. This integer
	 *	represents the index into the string where 'splitChar' was
	 *	found.
	 *
	 * Arguments:
	 *	maxLength: should be taken to be the number of characters
	 *		to search for 'splitChar' in. That is, maxLength bytes
	 *		will be searched for splitChar. If "path" is expected
	 *		to be NULL terminated, you may want to take that into
	 *		consideration.
	 **/
	status_t getIndexOfNext(
		utf8Char *path, utf8Char splitChar, uarch_t maxLength);

	/**	nodeC:
	 * Base type for all the vfs namespace types.
	 **********************************************************************/
	class nodeC
	{
	public:
		nodeC(void)
		:
		refCount(0)
		{}

		error_t initialize(void) { return ERROR_SUCCESS; }
		~nodeC(void) {}

	public:
		void incrementRefCountBy(ubit32 amount)
			{ atomicAsm::add((sarch_t *)&refCount, amount); }

		void decrementRefCountBy(ubit32 amount)
			{ atomicAsm::add((sarch_t *)&refCount, amount); }

		ubit32 getRefCount(void)
			{ return atomicAsm::read((sarch_t *)&refCount); }

	private:
		ubit32		refCount;
	};

	/**	tagC:
	 * Base type for all tag types, can be used in derived classes as the
	 * base for implementation.
	 *
	 * Can be either a normal directory, a mountpoint, file or a symlink.
	 * The inode type that it points to remains the same, but the 'type'
	 * value is used to tell the correct type to cast the inode pointer to.
	 **********************************************************************/
	class inodeC;
	
	template <ubit16 maxNameLength>
	class tagC
	:
	public nodeC
	{
	public:
		tagC(
			utf8Char *name,
			tagTypeE type,
			tagC *parent,
			inodeC *inode=__KNULL)
		:
		inode(inode), parent(parent), flags(0), type(type)
		{
			strncpy8(this->name, name, maxNameLength);
			this->name[maxNameLength-1] = '\0';
		}

		error_t initialize(void) { return nodeC::initialize(); }
		~tagC(void) {};

	public:
		error_t rename(utf8Char *newName);
		inodeC *getInode(void) { return inode; }
		utf8Char *getName(void) { return name; }
		tagC *getParent(void) { return parent; }
		ubit16 getMaxNameLength(void) { return maxNameLength; }
		tagTypeE getType(void) { return type; }

	public:
		inodeC			*inode;

	private:
		tagC			*parent;
		ubit32			flags;
		tagTypeE		type;
		/**	XXX:
		 * The "name" member must always remain the last in the struct.
		 *
		 * At some point in the future we may wish to support generic
		 * VFS tree listing, where a single iterator type can be used
		 * to iterate through the nodes in any of the VFSs in the kernel
		 * regardless of which VFS it is (storage VFS, device VFS, etc).
		 *
		 * In order to iterate through the tags in a "dirInodeC"'s list
		 * of sub-tags, we must choose a tag template with a
		 * "maxNameLength" that is sufficient to generically fit all the
		 * different VFS' max tag name lengths. This will probably be
		 * 256, or so.
		 *
		 * Consider the fact that for example, the device VFS has a max
		 * tag name length of about 16, and it becomes obvious why it's
		 * important to just ensure that the "name" member remains last.
		 **/
		utf8Char		name[maxNameLength];
	};

	/**	inodeC:
	 * Base inode type.
	 **********************************************************************/
	class inodeC
	:
	public nodeC
	{
	public:
		inodeC(void) {}
		error_t initialize(void) { return nodeC::initialize(); }
		~inodeC(void) {}
	};

	/**	dirInodeC:
	 * Base directory inode type. Holds a list of child tags.
	 **********************************************************************/
	template <class tagType>
	class dirInodeC
	:
	public inodeC
	{
	public:
		dirInodeC(void)
		:
		nDirs(0), nLeaves(0)
		{}

		error_t initialize(void);
		~dirInodeC(void) {};

	public:
		tagType *createDirTag(
			utf8Char *name,
			tagTypeE type,
			tagType *parent,
			dirInodeC *inode, error_t *err);

		tagType *createLeafTag(
			utf8Char *name,
			tagTypeE type,
			tagType *parent,
			inodeC *inode, error_t *err);

		sarch_t removeDirTag(utf8Char *name);
		sarch_t removeLeafTag(utf8Char *name);

		tagType *getDirTag(utf8Char *name);
		tagType *getLeafTag(utf8Char *name);

		//vfs::inodeTypeE getType(void) { return DIR; }

		void dumpDirs(void);
		void dumpLeaves(void);

	public:
		ptrListC<tagType>		dirs;
		ptrListC<tagType>		leaves;
		ubit32				nDirs;
		ubit32				nLeaves;
	};

	/**	EXPLANATION:
	 * A Currentt is an abstraction of a tree of named resources ("VFS").
	 * There are multiple currentts (VFSs) in Zambesii. We do not unify the
	 * various different VFSs into one tree. Files are in a separate VFS
	 * from devices, which are separate from distributaries, etc.
	 *
	 * For things like sockets, we may unify them into the hierarchical
	 * storage VFS, but it is more likely that we will create a new
	 * currentt for them.
	 **********************************************************************/
	class currenttC
	{
	public:
		currenttC(utf8Char prefix)
		:
		prefix(prefix)
		{}

		error_t initialize(void) { return ERROR_SUCCESS; }
		~currenttC(void) {}

	public:
		utf8Char getPrefix(void);

	private:
		utf8Char	prefix;
	};
}


/**	Template definitions for vfs::dirInodeC.
 ******************************************************************************/
template <class tagType>
error_t vfs::dirInodeC<tagType>::initialize(void)
{
	error_t		ret;

	ret = nodeC::initialize();
	if (ret != ERROR_SUCCESS) { return ret; };
	ret = dirs.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };
	return leaves.initialize();
}

template <class tagType>
tagType *vfs::dirInodeC<tagType>::createDirTag(
	utf8Char *name,
	tagTypeE type,
	tagType *parent,
	dirInodeC<tagType> *inode,
	error_t *err
	)
{
	tagType	*ret;

	if (name == __KNULL || err == __KNULL || parent == __KNULL)
	{
		if (err != __KNULL) { *err = ERROR_INVALID_ARG; };
		return __KNULL;
	};

	if (type == vfs::FILE || type == vfs::SYMLINK)
	{
		if (err != __KNULL) { *err = ERROR_UNSUPPORTED; }
		return __KNULL;
	};

	// Only utf8 can be used for VFS node names.
	if (name[0] == '\0')
	{
		*err = ERROR_INVALID_ARG_VAL;
		return __KNULL;
	};

	/* FIXME:
	 * Can't use object caching because of the structure of the VFS classes.
	 **/
	ret = new tagType(name, type, parent, inode);
	if (ret == __KNULL)
	{
		*err = ERROR_MEMORY_NOMEM;
		return __KNULL;
	};

	*err = ret->initialize();
	if (*err != ERROR_SUCCESS)
	{
		delete ret;
		return __KNULL;
	};

	*err = dirs.insert(ret);
	if (*err != ERROR_SUCCESS)
	{
		delete ret;
		return __KNULL;
	};

	nDirs++;
	return ret;
}

template <class tagType>
tagType *vfs::dirInodeC<tagType>::createLeafTag(
	utf8Char *name,
	tagTypeE type,
	tagType *parent,
	inodeC *inode,
	error_t *err
	)
{
	tagType	*ret;

	if (name == __KNULL || parent == __KNULL || err == __KNULL)
	{
		if (err != __KNULL) { *err = ERROR_INVALID_ARG; };
		return __KNULL;
	};

	if (type == vfs::DIR || type == vfs::MOUNTPOINT)
	{
		if (err != __KNULL) { *err = ERROR_UNSUPPORTED; }
		return __KNULL;
	};

	if (name[0] == '\0')
	{
		*err = ERROR_INVALID_ARG_VAL;
		return __KNULL;
	};

	/**	FIXME:
	 * Can't use object caching due to the class hierarchy.
	 **/
	ret = new tagType(name, type, parent, inode);
	if (ret == __KNULL)
	{
		*err = ERROR_MEMORY_NOMEM;
		return __KNULL;
	};

	*err = ret->initialize();
	if (*err != ERROR_SUCCESS)
	{
		delete ret;
		return __KNULL;
	};

	*err = leaves.insert(ret);
	if (*err != ERROR_SUCCESS)
	{
		delete ret;
		return __KNULL;
	};

	nLeaves++;
	return ret;
}

template <class tagType>
sarch_t vfs::dirInodeC<tagType>::removeDirTag(utf8Char *name)
{
	void			*handle;
	tagType		*currItem;

	if (name == __KNULL || name[0] == '\0') { return 0; };

	handle = __KNULL;
	dirs.lock();
	currItem = dirs.getNextItem(&handle, PTRLIST_FLAGS_NO_AUTOLOCK);
	for (; currItem != __KNULL;
		currItem = dirs.getNextItem(&handle, PTRLIST_FLAGS_NO_AUTOLOCK))
	{
		if (strncmp8(
			currItem->getName(), name,
			currItem->getMaxNameLength()) != 0)
		{
			continue;
		};

		// Found it.
		nDirs--;
		dirs.unlock();
		return dirs.remove(currItem);
	};

	dirs.unlock();
	return 0;
}

template <class tagType>
sarch_t vfs::dirInodeC<tagType>::removeLeafTag(utf8Char *name)
{
	void		*handle;
	tagType	*currItem;

	if (name == __KNULL || name[0] == '\0') { return 0; };

	handle = __KNULL;
	leaves.lock();
	currItem = leaves.getNextItem(&handle, PTRLIST_FLAGS_NO_AUTOLOCK);
	for (; currItem != __KNULL;
		currItem = leaves.getNextItem(
			&handle, PTRLIST_FLAGS_NO_AUTOLOCK))
	{
		if (strncmp8(
			currItem->getName(), name,
			currItem->getMaxNameLength()) != 0)
		{
			continue;
		};

		// Found it.
		nLeaves--;
		leaves.unlock();
		return leaves.remove(currItem);
	};

	leaves.unlock();
	return 0;
}

template <class tagType>
tagType *vfs::dirInodeC<tagType>::getDirTag(
	utf8Char *name
	)
{
	void			*handle;
	tagType		*currItem;

	if (name == __KNULL || name[0] == '\0') { return __KNULL; };

	handle = __KNULL;
	dirs.lock();
	currItem = dirs.getNextItem(&handle, PTRLIST_FLAGS_NO_AUTOLOCK);
	for (; currItem != __KNULL;
		currItem = dirs.getNextItem(&handle, PTRLIST_FLAGS_NO_AUTOLOCK))
	{
		if (strncmp8(
			currItem->getName(), name,
			currItem->getMaxNameLength()) != 0)
		{
			continue;
		};

		// Found it.
		dirs.unlock();
		return currItem;
	};

	dirs.unlock();
	return __KNULL;
}

template <class tagType>
tagType *vfs::dirInodeC<tagType>::getLeafTag(
	utf8Char *name
	)
{
	void			*handle;
	tagType		*currItem;

	if (name == __KNULL || name[0] == '\0') { return __KNULL; };

	handle = __KNULL;
	leaves.lock();
	currItem = leaves.getNextItem(&handle, PTRLIST_FLAGS_NO_AUTOLOCK);
	for (; currItem != __KNULL;
		currItem = leaves.getNextItem(
			&handle, PTRLIST_FLAGS_NO_AUTOLOCK))
	{
		if (strncmp8(
			currItem->getName(), name,
			currItem->getMaxNameLength()) != 0)
		{
			continue;
		};

		// Found it.
		leaves.unlock();
		return currItem;
	};

	leaves.unlock();
	return __KNULL;
}

template <class tagType>
void vfs::dirInodeC<tagType>::dumpDirs(void)
{
	void			*handle;
	tagType		*currItem;

	__kprintf(NOTICE"vfs::dirInode: dumping dirs (%d total).\n", nDirs);
	handle = __KNULL;
	dirs.lock();
	currItem = dirs.getNextItem(&handle, PTRLIST_FLAGS_NO_AUTOLOCK);
	for (; currItem != __KNULL;
		currItem = dirs.getNextItem(&handle, PTRLIST_FLAGS_NO_AUTOLOCK))
	{
		__kprintf(CC"\tname: %s. Parent 0x%p.\n",
			currItem->getName(), currItem->getParent());
	};

	dirs.unlock();
}

template <class tagType>
void vfs::dirInodeC<tagType>::dumpLeaves(void)
{
	void			*handle;
	tagType		*currItem;

	__kprintf(NOTICE"vfs::dirInode: dumping leaves (%d total).\n", nLeaves);
	handle = __KNULL;
	leaves.lock();
	currItem = leaves.getNextItem(&handle, PTRLIST_FLAGS_NO_AUTOLOCK);
	for (; currItem != __KNULL;
		currItem = leaves.getNextItem(&handle, PTRLIST_FLAGS_NO_AUTOLOCK))
	{
		__kprintf(CC"\tname: %s. Parent 0x%p.\n",
			currItem->getName(), currItem->getParent());
	};

	leaves.unlock();
}

/**	Template definitions for vfs::tagC.
 ******************************************************************************/
template <ubit16 maxNameLength>
error_t vfs::tagC<maxNameLength>::rename(utf8Char *name)
{
	if (name == __KNULL) { return ERROR_INVALID_ARG; };
	if (name[0] == '\0') { return ERROR_INVALID_ARG_VAL; };

	strncpy8(this->name, name, maxNameLength);
	this->name[maxNameLength-1] = '\0';
}

#endif

