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

	template <class inodeType, ubit16 maxNameLength>
	class tagC
	:
	public nodeC
	{
	public:
		tagC(utf8Char *name, tagC *parent, inodeType *inode=__KNULL)
		:
		parent(parent), inode(inode), flags(0)
		{
			strncpy8(this->name, name, maxNameLength);
		}

		error_t initialize(void) { return nodeC::initialize(); }
		~tagC(void) {};

	public:
		void rename(utf8Char *newName);
		inodeType *getInode(void) { return inode; }
		utf8Char *getName(void) { return name; }

	public:
		utf8Char		name[maxNameLength];
		tagC			*parent;
		inodeType		*inode;
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
	};

	template <ubit16 maxNameLength>
	class dirInodeC
	:
	public nodeC
	{
	public:
		dirInodeC(void)
		:
		nDirs(0), nLeaves(0)
		{}

		error_t initialize(void)
		{
			error_t		ret;

			ret = nodeC::initialize();
			if (ret != ERROR_SUCCESS) { return ret; };
			ret = dirs.initialize();
			if (ret != ERROR_SUCCESS) { return ret; };
			return leaves.initialize();
		}

		~dirInodeC(void) {};

	public:
		tagC<dirInodeC, maxNameLength> *createDirTag(
			utf8Char *name, error_t *err);

		tagC<leafInodeC, maxNameLength> *createLeafTag(
			utf8Char *name, error_t *err);

		sarch_t removeDirTag(utf8Char *name);
		sarch_t removeLeafTag(utf8Char *name);

		tagC<dirInodeC, maxNameLength> *getDirTag(utf8Char *name);
		tagC<leafInodeC, maxNameLength> *getLeafTag(utf8Char *name);

		void dumpDirs(void);
		void dumpLeaves(void);

	public:
		ptrListC< tagC<dirInodeC, maxNameLength> >	dirs;
		ptrListC< tagC<leafInodeC, maxNameLength> >	leaves;
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

#endif

