#ifndef _FLOODPLAINN_VFS_H
	#define _FLOODPLAINN_VFS_H

	#include <__kstdlib/__ktypes.h>
	#include <chipset/zkcm/zkcmCore.h>
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
namespace fplainn { class Device; }

namespace fvfs
{
	class Currentt;

	/**	hvfs::Tag:
	 * Base type for a device that has child devices.
	 **********************************************************************/
	class Tag
	:
	public vfs::Tag<FVFS_TAG_NAME_MAXLEN>
	{
	friend class vfs::DirInode<Tag>;
	friend class Currentt;
		/* Private constructor only for use within vfs::DirInode.
		 * It is used to allow vfs::DirInode::create*Tag() to
		 * generically create new tags of this type.
		 **/
		Tag(
			utf8Char *name, vfs::tagTypeE,
			Tag *parent, vfs::Inode *inode=NULL)
		:
		vfs::Tag<FVFS_TAG_NAME_MAXLEN>(
			name, vfs::DEVICE, parent, inode)
		{}

		error_t initialize(void)
		{
			error_t		ret;

			ret = vfs::Tag<FVFS_TAG_NAME_MAXLEN>::initialize();
			if (ret != ERROR_SUCCESS) { return ret; };
			zkcmCore.chipsetEventNotification(
				__KPOWER_EVENT_FVFS_AVAIL, 0);

			return ERROR_SUCCESS;
		}

		~Tag(void) {}

	public:
		fplainn::Device *getInode(void)
			{ return (fplainn::Device *)inode; }
	};

	class Currentt
	:
	public vfs::Currentt
	{
	public:
		Currentt(void);
		error_t initialize(void);
		~Currentt(void) {}

	public:
		Tag *getRoot(void) { return &rootTag; }

		error_t getPath(utf8Char *path, Tag **ret);

	private:
		Tag		rootTag;
	};
}

#endif

