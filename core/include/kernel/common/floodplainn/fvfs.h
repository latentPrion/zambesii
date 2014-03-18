#ifndef _FLOODPLAINN_VFS_H
	#define _FLOODPLAINN_VFS_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/vfsTrib/vfsTypes.h>

#define FVFS_TAG_NAME_MAXLEN		(32)
#define FVFS_PATH_MAXLEN		(192)

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
namespace fplainn { class deviceC; }

namespace fvfs
{
	class currenttC;

	/**	hvfs::tagC:
	 * Base type for a device that has child devices.
	 **********************************************************************/
	class tagC
	:
	public vfs::tagC<FVFS_TAG_NAME_MAXLEN>
	{
	friend class vfs::dirInodeC<tagC>;
	friend class currenttC;
		/* Private constructor only for use within vfs::dirInodeC.
		 * It is used to allow vfs::dirInodeC::create*Tag() to
		 * generically create new tags of this type.
		 **/
		tagC(
			utf8Char *name, vfs::tagTypeE,
			tagC *parent, vfs::inodeC *inode=NULL)
		:
		vfs::tagC<FVFS_TAG_NAME_MAXLEN>(
			name, vfs::DEVICE, parent, inode)
		{}

		error_t initialize(void)
		{
			return vfs::tagC<FVFS_TAG_NAME_MAXLEN>::initialize();
		}

		~tagC(void) {}

	public:
		fplainn::deviceC *getInode(void)
			{ return (fplainn::deviceC *)inode; }
	};

	class currenttC
	:
	public vfs::currenttC
	{
	public:
		currenttC(void);
		error_t initialize(void);
		~currenttC(void) {}

	public:
		tagC *getRoot(void) { return &rootTag; }

		error_t getPath(utf8Char *path, tagC **ret);

	private:
		tagC		rootTag;
	};
}

#endif

