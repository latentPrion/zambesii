#ifndef _VIRTUAL_FILE_SYSTEM_TYPES_H
	#define _VIRTUAL_FILE_SYSTEM_TYPES_H

	#include <arch/cpuControl.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kclib/string.h>
	#include <__kstdlib/__kclib/string8.h>
	#include <__kclasses/heapList.h>
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

	/**	Node:
	 * Base type for all the vfs namespace types.
	 **********************************************************************/
	class Node
	{
	public:
		Node(void)
		:
		refCount(0)
		{}

		error_t initialize(void) { return ERROR_SUCCESS; }
		~Node(void) {}

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

	/**	Tag:
	 * Base type for all tag types, can be used in derived classes as the
	 * base for implementation.
	 *
	 * Can be either a normal directory, a mountpoint, file or a symlink.
	 * The inode type that it points to remains the same, but the 'type'
	 * value is used to tell the correct type to cast the inode pointer to.
	 **********************************************************************/
	class Inode;

	template <ubit16 maxNameLength>
	class Tag
	:
	public Node
	{
	public:
		Tag(
			utf8Char *name,
			tagTypeE type,
			Tag *parent,
			Inode *inode=NULL)
		:
		inode(inode), parent(parent), flags(0), type(type)
		{
			strncpy8(this->name, name, maxNameLength);
			this->name[maxNameLength-1] = '\0';
		}

		error_t initialize(void) { return Node::initialize(); }
		~Tag(void) {};

	public:
		error_t rename(utf8Char *newName);
		Inode *getInode(void) { return inode; }
		utf8Char *getName(void) { return name; }
		Tag *getParent(void) { return parent; }
		tagTypeE getType(void) { return type; }

		ubit16 getMaxNameLength(void) { return maxNameLength; }
		error_t getFullName(utf8Char *strmem, uarch_t strlen);

	public:
		Inode			*inode;

	private:
		Tag			*parent;
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
		 * In order to iterate through the tags in a "DirInode"'s list
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

	/**	Inode:
	 * Base inode type.
	 **********************************************************************/
	class Inode
	:
	public Node
	{
	public:
		Inode(void) {}
		error_t initialize(void) { return Node::initialize(); }
		~Inode(void) {}
	};

	/**	DirInode:
	 * Base directory inode type. Holds a list of child tags.
	 **********************************************************************/
	template <class tagType>
	class DirInode
	:
	public Inode
	{
	public:
		DirInode(void) {}

		error_t initialize(void);
		~DirInode(void) {};

	public:
		tagType *createDirTag(
			utf8Char *name,
			tagTypeE type,
			tagType *parent,
			DirInode *inode, error_t *err);

		tagType *createLeafTag(
			utf8Char *name,
			tagTypeE type,
			tagType *parent,
			Inode *inode, error_t *err);

		sarch_t removeDirTag(utf8Char *name);
		sarch_t removeLeafTag(utf8Char *name);

		tagType *getDirTag(utf8Char *name);
		tagType *getLeafTag(utf8Char *name);

		//vfs::inodeTypeE getType(void) { return DIR; }

		void dumpDirs(void);
		void dumpLeaves(void);

	public:
		HeapList<tagType>		dirs;
		HeapList<tagType>		leaves;
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
	class Currentt
	{
	public:
		Currentt(utf8Char prefix)
		:
		prefix(prefix)
		{}

		error_t initialize(void) { return ERROR_SUCCESS; }
		~Currentt(void) {}

	public:
		utf8Char getPrefix(void);

	private:
		utf8Char	prefix;
	};
}


/**	Template definitions for vfs::DirInode.
 ******************************************************************************/
template <class tagType>
error_t vfs::DirInode<tagType>::initialize(void)
{
	error_t		ret;

	ret = Node::initialize();
	if (ret != ERROR_SUCCESS) { return ret; };
	ret = dirs.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };
	return leaves.initialize();
}

template <class tagType>
tagType *vfs::DirInode<tagType>::createDirTag(
	utf8Char *name,
	tagTypeE type,
	tagType *parent,
	DirInode<tagType> *inode,
	error_t *err
	)
{
	tagType	*ret;

	if (name == NULL || err == NULL || parent == NULL)
	{
		if (err != NULL) { *err = ERROR_INVALID_ARG; };
		return NULL;
	};

	if (type == vfs::FILE || type == vfs::SYMLINK)
	{
		if (err != NULL) { *err = ERROR_UNSUPPORTED; }
		return NULL;
	};

	// Only utf8 can be used for VFS node names.
	if (name[0] == '\0')
	{
		*err = ERROR_INVALID_ARG_VAL;
		return NULL;
	};

	/* FIXME:
	 * Can't use object caching because of the structure of the VFS classes.
	 **/
	ret = new tagType(name, type, parent, inode);
	if (ret == NULL)
	{
		*err = ERROR_MEMORY_NOMEM;
		return NULL;
	};

	*err = ret->initialize();
	if (*err != ERROR_SUCCESS)
	{
		delete ret;
		return NULL;
	};

	*err = dirs.insert(ret);
	if (*err != ERROR_SUCCESS)
	{
		delete ret;
		return NULL;
	};

	return ret;
}

template <class tagType>
tagType *vfs::DirInode<tagType>::createLeafTag(
	utf8Char *name,
	tagTypeE type,
	tagType *parent,
	Inode *inode,
	error_t *err
	)
{
	tagType	*ret;

	if (name == NULL || parent == NULL || err == NULL)
	{
		if (err != NULL) { *err = ERROR_INVALID_ARG; };
		return NULL;
	};

	if (type == vfs::DIR || type == vfs::MOUNTPOINT)
	{
		if (err != NULL) { *err = ERROR_UNSUPPORTED; }
		return NULL;
	};

	if (name[0] == '\0')
	{
		*err = ERROR_INVALID_ARG_VAL;
		return NULL;
	};

	/**	FIXME:
	 * Can't use object caching due to the class hierarchy.
	 **/
	ret = new tagType(name, type, parent, inode);
	if (ret == NULL)
	{
		*err = ERROR_MEMORY_NOMEM;
		return NULL;
	};

	*err = ret->initialize();
	if (*err != ERROR_SUCCESS)
	{
		delete ret;
		return NULL;
	};

	*err = leaves.insert(ret);
	if (*err != ERROR_SUCCESS)
	{
		delete ret;
		return NULL;
	};

	return ret;
}

template <class tagType>
sarch_t vfs::DirInode<tagType>::removeDirTag(utf8Char *name)
{
	tagType					*currItem;

	if (name == NULL || name[0] == '\0') { return 0; };

	typename HeapList<tagType>::Iterator		it =
		dirs.begin(PTRLIST_FLAGS_NO_AUTOLOCK);

	dirs.lock();
	for (; it != dirs.end(); ++it)
	{
		currItem = *it;

		if (strncmp8(
			currItem->getName(), name,
			currItem->getMaxNameLength()) != 0)
		{
			continue;
		};

		// Found it.
		dirs.unlock();
		return dirs.remove(currItem);
	};

	dirs.unlock();
	return 0;
}

template <class tagType>
sarch_t vfs::DirInode<tagType>::removeLeafTag(utf8Char *name)
{
	tagType		*currItem;

	if (name == NULL || name[0] == '\0') { return 0; };

	typename HeapList<tagType>::Iterator		it =
		leaves.begin(PTRLIST_FLAGS_NO_AUTOLOCK);

	leaves.lock();
	for (; it != leaves.end(); ++it)
	{
		currItem = *it;

		if (strncmp8(
			currItem->getName(), name,
			currItem->getMaxNameLength()) != 0)
		{
			continue;
		};

		// Found it.
		leaves.unlock();
		return leaves.remove(currItem);
	};

	leaves.unlock();
	return 0;
}

template <class tagType>
tagType *vfs::DirInode<tagType>::getDirTag(
	utf8Char *name
	)
{
	tagType					*currItem;

	if (name == NULL || name[0] == '\0') { return NULL; };

	typename HeapList<tagType>::Iterator		it =
		dirs.begin(PTRLIST_FLAGS_NO_AUTOLOCK);

	dirs.lock();
	for (; it != dirs.end(); ++it)
	{
		currItem = *it;

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
	return NULL;
}

template <class tagType>
tagType *vfs::DirInode<tagType>::getLeafTag(
	utf8Char *name
	)
{
	tagType			*currItem;

	if (name == NULL || name[0] == '\0') { return NULL; };

	typename HeapList<tagType>::Iterator		it =
		leaves.begin(PTRLIST_FLAGS_NO_AUTOLOCK);

	leaves.lock();
	for (; it != leaves.end(); ++it)
	{
		currItem = *it;

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
	return NULL;
}

template <class tagType>
void vfs::DirInode<tagType>::dumpDirs(void)
{
	tagType		*currItem;

	printf(NOTICE"vfs::dirInode: dumping dirs (%d total).\n",
		dirs.getNItems());

	typename HeapList<tagType>::Iterator		it =
		dirs.begin(PTRLIST_FLAGS_NO_AUTOLOCK);

	dirs.lock();
	for (; it != dirs.end(); ++it)
	{
		currItem = *it;

		printf(CC"\tname: %s. Parent 0x%p.\n",
			currItem->getName(), currItem->getParent());
	};

	dirs.unlock();
}

template <class tagType>
void vfs::DirInode<tagType>::dumpLeaves(void)
{
	tagType		*currItem;

	printf(NOTICE"vfs::dirInode: dumping leaves (%d total).\n",
		leaves.getNItems());

	typename HeapList<tagType>::Iterator			it =
		leaves.begin(PTRLIST_FLAGS_NO_AUTOLOCK);

	leaves.lock();
	for (; it != leaves.end(); ++it)
	{
		currItem = *it;

		printf(CC"\tname: %s. Parent 0x%p.\n",
			currItem->getName(), currItem->getParent());
	};

	leaves.unlock();
}

/**	Template definitions for vfs::Tag.
 ******************************************************************************/
template <ubit16 maxNameLength>
error_t vfs::Tag<maxNameLength>::rename(utf8Char *name)
{
	if (name == NULL) { return ERROR_INVALID_ARG; };
	if (name[0] == '\0') { return ERROR_INVALID_ARG_VAL; };

	strncpy8(this->name, name, maxNameLength);
	this->name[maxNameLength-1] = '\0';
}

template <ubit16 maxNameLength>
error_t vfs::Tag<maxNameLength>::getFullName(
	utf8Char *buffer, uarch_t maxBuffLen
	)
{
	Tag		*currTag;
	uarch_t		index, totalRequiredMem, segmentLen;
	sbit8		maxBuffLenExceeded;

	/**	EXPLANATION:
	 * Given a tag, traverse backward through the VFS and incrementally
	 * build the full path of the tag.
	 *
	 * What we do is we start at the end of the supplied output buffer,
	 * and copy the tag names, tacking them on leftwards. This will produce,
	 * at the end of the operation, a string with a big gap at its front,
	 * and the actual filename all the way at the end of the buffer. Then we
	 * just move the filename back to the beginning of the buffer, and
	 * return.
	 **/
	// We can't work with a 0 length buffer.
	if (maxBuffLen == 0) { return ERROR_INVALID_ARG; };

	// Start by null terminating the buffer.
	buffer[maxBuffLen - 1] = '\0';
	index = maxBuffLen - 1;

	totalRequiredMem = 1;
	maxBuffLenExceeded = 0;

	for (currTag = this; currTag->parent != currTag;
		currTag = currTag->parent, index -= segmentLen + 1)
	{
		segmentLen = strlen8(currTag->name);
		totalRequiredMem += segmentLen + 1;
		if (maxBuffLenExceeded) { continue; };

		/* If the current tag's strlen is greater than the amount of
		 * buffer space we have left, set a flag stating that the
		 * operation has exceeded its buffer.
		 *
		 * When this flag is set, we no longer continue building the
		 * string, but our aim changes to tallying the total amount of
		 * memory required to successfully complete this function call.
		 * We then return that total amount to the caller at the end.
		 ***/
		if (segmentLen + 1 > index)
			{ maxBuffLenExceeded = 1; continue; };

		if (segmentLen > 0)
		{
			buffer[index - segmentLen - 1] =  '/';
			strncpy8(
				&buffer[index - segmentLen],
				currTag->name, segmentLen);
		};
	};

	if (maxBuffLenExceeded) { return (signed)totalRequiredMem; }
	else
	{
		// Now we move the name up to the start of the buffer.
		memmove(buffer, &buffer[index], maxBuffLen - index);
		return ERROR_SUCCESS;
	};
}

#endif
