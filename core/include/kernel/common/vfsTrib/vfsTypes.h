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
	enum inodeTypeE { DIR=1, LEAF };

	/* Returns:
	 *	ERROR_NOT_FOUND: if no instance of 'splitChar' is found before
	 *		the first instance of '\0'.
	 *	ERROR_INVALID_RESOURCE_NAME: If no instance of 'splitChar' is
	 *		found before 'maxLength' characters have been searched.
	 *	A signed integer if 'splitChar' is found. This integer
	 *	represents the index into the string where 'splitChar' was
	 *	found.
	 **/
	status_t getIndexOfNext(
		utf8Char *path, utf8Char splitChar, uarch_t maxLength);

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

	template <
		class internalInodeType, class dirInodeType,
		ubit16 maxNameLength>
	class tagC
	:
	public nodeC
	{
	public:
		tagC(
			utf8Char *name,
			tagC<dirInodeType, dirInodeType, maxNameLength> *parent,
			internalInodeType *inode=__KNULL)
		:
		parent(parent), inode(inode), flags(0)
		{
			strncpy8(this->name, name, maxNameLength);
			this->name[maxNameLength-1] = '\0';
		}

		error_t initialize(void) { return nodeC::initialize(); }
		~tagC(void) {};

	public:
		error_t rename(utf8Char *newName);
		internalInodeType *getInode(void) { return inode; }
		utf8Char *getName(void) { return name; }
		tagC<dirInodeType, dirInodeType, maxNameLength> *getParent(void)
			{ return parent; }

	public:
		utf8Char		name[maxNameLength];
		tagC<dirInodeType, dirInodeType, maxNameLength>	*parent;
		internalInodeType	*inode;
		ubit32			flags;
	};

	class leafInodeC
	:
	public nodeC
	{
	public:
		leafInodeC(void) {};
		error_t initialize(void) { return nodeC::initialize(); }
		~leafInodeC(void) {};

	public:
		vfs::inodeTypeE getType(void) { return LEAF; }
	};

	template <class dirInodeType, class leafInodeType, ubit16 maxNameLength>
	class dirInodeC
	:
	public nodeC
	{
	public:
		dirInodeC(void)
		:
		nDirs(0), nLeaves(0)
		{}

		error_t initialize(void);
		~dirInodeC(void) {};

	public:
		virtual tagC<dirInodeType, dirInodeType, maxNameLength> *
		createDirTag(
			utf8Char *name,
			tagC<dirInodeType, dirInodeType, maxNameLength> *parent,
			dirInodeType *inode, error_t *err);

		virtual tagC<leafInodeType, dirInodeType, maxNameLength> *
		createLeafTag(
			utf8Char *name,
			tagC<dirInodeType, dirInodeType, maxNameLength> *parent,
			leafInodeType *inode, error_t *err);

		virtual sarch_t removeDirTag(utf8Char *name);
		virtual sarch_t removeLeafTag(utf8Char *name);

		virtual tagC<dirInodeType, dirInodeType, maxNameLength> *
		getDirTag(utf8Char *name);

		virtual tagC<leafInodeType, dirInodeType, maxNameLength> *
		getLeafTag(utf8Char *name);

		vfs::inodeTypeE getType(void) { return DIR; }

		void dumpDirs(void);
		void dumpLeaves(void);

	public:
		ptrListC< tagC<dirInodeType, dirInodeType, maxNameLength> >
			dirs;

		ptrListC< tagC<leafInodeType, dirInodeType, maxNameLength> >
			leaves;

		ubit32			nDirs;
		ubit32			nLeaves;
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
	 **/
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
template <class dirInodeType, class leafInodeType, ubit16 maxNameLength>
error_t vfs::dirInodeC<dirInodeType, leafInodeType, maxNameLength>
	::initialize(void)
{
	error_t		ret;

	ret = nodeC::initialize();
	if (ret != ERROR_SUCCESS) { return ret; };
	ret = dirs.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };
	return leaves.initialize();
}

template <class dirInodeType, class leafInodeType, ubit16 maxNameLength>
vfs::tagC<dirInodeType, dirInodeType, maxNameLength> *
vfs::dirInodeC<dirInodeType, leafInodeType, maxNameLength>::createDirTag(
	utf8Char *name,
	vfs::tagC<dirInodeType, dirInodeType, maxNameLength> *parent,
	dirInodeType *inode,
	error_t *err
	)
{
	vfs::tagC<dirInodeType, dirInodeType, maxNameLength>	*ret;

	if (name == __KNULL || err == __KNULL || parent == __KNULL)
	{
		if (err != __KNULL) { *err = ERROR_INVALID_ARG; };
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
	ret = new vfs::tagC<dirInodeType, dirInodeType, maxNameLength>(
		name, parent, inode);

	if (ret == __KNULL)
	{
		*err = ERROR_MEMORY_NOMEM;
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

template <class dirInodeType, class leafInodeType, ubit16 maxNameLength>
vfs::tagC<leafInodeType, dirInodeType, maxNameLength> *
vfs::dirInodeC<dirInodeType, leafInodeType, maxNameLength>::createLeafTag(
	utf8Char *name,
	vfs::tagC<dirInodeType, dirInodeType, maxNameLength> *parent,
	leafInodeType *inode,
	error_t *err
	)
{
	vfs::tagC<leafInodeType, dirInodeType, maxNameLength>	*ret;

	if (name == __KNULL || parent == __KNULL || err == __KNULL)
	{
		if (err != __KNULL) { *err = ERROR_INVALID_ARG; };
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
	ret = new vfs::tagC<leafInodeType, dirInodeType, maxNameLength>(
		name, parent, inode);

	if (ret == __KNULL)
	{
		*err = ERROR_MEMORY_NOMEM;
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

template <class dirInodeType, class leafInodeType, ubit16 maxNameLength>
sarch_t
vfs::dirInodeC<dirInodeType, leafInodeType, maxNameLength>::removeDirTag(
	utf8Char *name
	)
{
	void							*handle;
	vfs::tagC<dirInodeType, dirInodeType, maxNameLength>	*currItem;

	if (name == __KNULL || name[0] == '\0') { return 0; };

	handle = __KNULL;
	dirs.lock();
	currItem = dirs.getNextItem(&handle, PTRLIST_FLAGS_NO_AUTOLOCK);
	for (; currItem != __KNULL;
		currItem = dirs.getNextItem(&handle, PTRLIST_FLAGS_NO_AUTOLOCK))
	{
		if (strncmp8(currItem->getName(), name, maxNameLength) != 0) {
			continue;
		};

		// Found it.
		dirs.unlock();
		return dirs.remove(currItem);
	};

	dirs.unlock();
	return 0;
}

template <class dirInodeType, class leafInodeType, ubit16 maxNameLength>
sarch_t
vfs::dirInodeC<dirInodeType, leafInodeType, maxNameLength>::removeLeafTag(
	utf8Char *name
	)
{
	void							*handle;
	vfs::tagC<leafInodeType, dirInodeType, maxNameLength>	*currItem;

	if (name == __KNULL || name[0] == '\0') { return 0; };

	handle = __KNULL;
	leaves.lock();
	currItem = leaves.getNextItem(&handle, PTRLIST_FLAGS_NO_AUTOLOCK);
	for (; currItem != __KNULL;
		currItem = leaves.getNextItem(
			&handle, PTRLIST_FLAGS_NO_AUTOLOCK))
	{
		if (strncmp8(currItem->getName(), name, maxNameLength) != 0) {
			continue;
		};

		// Found it.
		leaves.unlock();
		return leaves.remove(currItem);
	};

	leaves.unlock();
	return 0;
}

template <class dirInodeType, class leafInodeType, ubit16 maxNameLength>
vfs::tagC<dirInodeType, dirInodeType, maxNameLength> *
vfs::dirInodeC<dirInodeType, leafInodeType, maxNameLength>::getDirTag(
	utf8Char *name
	)
{
	void						*handle;
	vfs::tagC<dirInodeType, dirInodeType, maxNameLength>	*currItem;

	if (name == __KNULL || name[0] == '\0') { return __KNULL; };

	handle = __KNULL;
	dirs.lock();
	currItem = dirs.getNextItem(&handle, PTRLIST_FLAGS_NO_AUTOLOCK);
	for (; currItem != __KNULL;
		currItem = dirs.getNextItem(&handle, PTRLIST_FLAGS_NO_AUTOLOCK))
	{
		if (strncmp8(currItem->getName(), name, maxNameLength) != 0) {
			continue;
		};

		// Found it.
		dirs.unlock();
		return currItem;
	};

	dirs.unlock();
	return __KNULL;
}

template <class dirInodeType, class leafInodeType, ubit16 maxNameLength>
vfs::tagC<leafInodeType, dirInodeType, maxNameLength> *
vfs::dirInodeC<dirInodeType, leafInodeType, maxNameLength>::getLeafTag(
	utf8Char *name
	)
{
	void					*handle;
	vfs::tagC<leafInodeType, dirInodeType, maxNameLength>	*currItem;

	if (name == __KNULL || name[0] == '\0') { return __KNULL; };

	handle = __KNULL;
	leaves.lock();
	currItem = leaves.getNextItem(&handle, PTRLIST_FLAGS_NO_AUTOLOCK);
	for (; currItem != __KNULL;
		currItem = leaves.getNextItem(
			&handle, PTRLIST_FLAGS_NO_AUTOLOCK))
	{
		if (strncmp8(currItem->getName(), name, maxNameLength) != 0) {
			continue;
		};

		// Found it.
		leaves.unlock();
		return currItem;
	};

	leaves.unlock();
	return __KNULL;
}

template <class dirInodeType, class leafInodeType, ubit16 maxNameLength>
void vfs::dirInodeC<dirInodeType, leafInodeType, maxNameLength>::dumpDirs(void)
{
	void							*handle;
	vfs::tagC<dirInodeType, dirInodeType, maxNameLength>	*currItem;

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

template <class dirInodeType, class leafInodeType, ubit16 maxNameLength>
void vfs::dirInodeC<dirInodeType, leafInodeType, maxNameLength>::dumpLeaves(void)
{
	void							*handle;
	vfs::tagC<leafInodeType, dirInodeType, maxNameLength>	*currItem;

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
template <class internalInodeType, class dirInodeType, ubit16 maxNameLength>
error_t vfs::tagC<internalInodeType, dirInodeType, maxNameLength>::rename(
	utf8Char *name
	)
{
	if (name == __KNULL) { return ERROR_INVALID_ARG; };
	if (name[0] == '\0') { return ERROR_INVALID_ARG_VAL; };

	strncpy8(this->name, name, maxNameLength);
	this->name[maxNameLength-1] = '\0';
}

#endif

