#ifndef _FLOODPLAINN_VFS_H
	#define _FLOODPLAINN_VFS_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/vfsTrib/vfsTypes.h>

#define FVFS_TAG_NAME_MAXLEN		(64)

/**	EXPLANATION:
 * The Floodplainn VFS is the abstraction used to provide a namespace of
 * objects that reflects the device tree hierarchy, making the device tree
 * easily accessible to userspace, and easier to work with from kernelspace.
 *
 * The interesting thing about the floodplainn VFS (FVFS) compared to the
 * storage VFSs (HVFS and TVFS) is that every node is a "folder" node, since
 * any device can have children. The FVFS itself does not provide for the
 * creation of "leaf" nodes since due to hotplug events, even a device which
 * previously stated that it had no children may later on advertise children
 * to the kernel. As a result of this, we deliberately overload and privatise
 * the leaf-tag creation/manipulation VFS base functions so that they cannot be
 * used.
 *
 * Every node in the FVFS has (1) a name-string, (2) a device object that it
 * points to and (3) a list of child nodes.
 **/

class deviceC;

namespace fvfs
{
	/**	hvfs::tagC:
	 * Base type for a device that has child devices.
	 **********************************************************************/
	class tagC
	:
	public vfs::tagC<FVFS_TAG_NAME_MAXLEN>, public vfs::dirInodeC<tagC>
	{
	friend class vfs::dirInodeC<tagC>;
		// Private constructor only for use within vfs::dirInodeC.
		tagC(
			utf8Char *name, vfs::tagTypeE,
			tagC *parent, vfs::inodeC *inode=__KNULL)
		:
		vfs::tagC<FVFS_TAG_NAME_MAXLEN>(
			name, vfs::DIR, parent, inode)
		{}

	public:
		tagC(utf8Char *name, tagC *parent, deviceC *device)
		:
		vfs::tagC<FVFS_TAG_NAME_MAXLEN>(
			name, vfs::DIR, parent, (vfs::inodeC *)device)
		{}

		error_t initialize(void)
		{
			error_t		ret;

			ret = vfs::dirInodeC<tagC>::initialize();
			if (ret != ERROR_SUCCESS) { return ret; }
			return vfs::tagC<FVFS_TAG_NAME_MAXLEN>::initialize();
		}
		~tagC(void) {}

	public:
		deviceC *getDevice(void) { return (deviceC *)getInode(); }

		/**	EXPLANATION:
		 * These are the publicly exposed wrappers around the underlying
		 * vfs::dirInodeC:: namespace methods. We hid the *DirTag()
		 * functions with overloads, and then "renamed" them to
		 * *Child() so we could have more intuitive naming, and more
		 * suitable function prototypes.
		 **/
		tagC *createChild(
			utf8Char *name, deviceC *device, error_t *err)
		{
			return createDirTag(
				name, vfs::DIR, this,
				(vfs::dirInodeC<tagC> *)device, err);
		}

		sarch_t removeChild(utf8Char *name)
			{ return removeDirTag(name); }

		tagC *getChild(utf8Char *name) { return getDirTag(name); }

	private:
		/* Not meant to be used by callers. Used only internally as
		 * wrapper functions. Deliberately made private.
		 **/
		tagC *createDirTag(
			utf8Char *name, vfs::tagTypeE type,
			tagC *parent,
			vfs::dirInodeC<tagC> *inode, error_t *err)
		{
			return vfs::dirInodeC<tagC>::createDirTag(
				name, type, parent, inode, err);
		}

		sarch_t removeDirTag(utf8Char *name)
			{ return vfs::dirInodeC<tagC>::removeDirTag(name); }

		tagC *getDirTag(utf8Char *name)
			{ return vfs::dirInodeC<tagC>::getDirTag(name); }

		/* The leaf functions are not meant to be used at all by anyone;
		 * not even internals within this class, and they override the
		 * base-class functions of the same name, returning error
		 * unconditionally.
		 **/
		tagC *createLeafTag(
			utf8Char *, vfs::tagTypeE, tagC *,
			vfs::inodeC *, error_t *err)
		{
			*err = ERROR_UNIMPLEMENTED;
			return __KNULL;
		}

		sarch_t removeLeafTag(utf8Char *) { return 0; }
		tagC *getLeafTag(utf8Char *) { return __KNULL; }
	};
		
}

#endif

