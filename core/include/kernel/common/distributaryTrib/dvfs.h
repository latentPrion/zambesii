#ifndef _DISTRIBUTARY_VFS_H
	#define _DISTRIBUTARY_VFS_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kclib/string8.h>
	#include <__kclasses/singleWaiterQueue.h>
	#include <chipset/zkcm/zkcmCore.h>
	#include <kernel/common/vfsTrib/vfsTypes.h>

#define DVFS_TAG_NAME_MAXLEN			(48)
#define DVFS_PATH_MAXLEN			(128)

#define DVFS_DINODE_VENDOR_MAXLEN		(64)
#define DVFS_DINODE_VENDORCONTACT_MAXLEN	(128)
#define DVFS_DINODE_DESCRIPTION_MAXLEN		(256)

#define DVFS_DINODE_FLAGS_DEFAULT		DVFS_DESCRIPTOR_FLAGS_DEFAULT

class DistributaryTrib;

namespace dvfs
{
	/**	sDistributaryDescriptor:
	 * Exported by every distributary packaged with the kernel. Used by
	 * the kernel to create the Distributary VFS.
	 **********************************************************************/
	struct sDistributaryDescriptor
	{
		/**	EXPLANATION:
		 * The macro constant that determines the maximum length of each
		 * string field is given after the field name.
		 *
		 * For the "categories" string array, the rationale is that a
		 * distributary can provide more than one service. For example,
		 * GFX servers often also handle input devices. So most
		 * GFX servers will list categories for "graphics",
		 * "co-ordinate input" and "character input", and not just
		 * "graphics".
		 *
		 *	VALID CATEGORY NAMES:
		 * "video output", "video input",
		 * "co-ordinate input", "character input",
		 * "networking", "storage",
		 * "audio output", "audio input"
		 *
		 * As indicated above, in many cases, a single distributary
		 * will provide multiple services. E.g., an audio server will
		 * probably handle both "audio input" and "audio output".
		 **/
		utf8Char	*name, // DVFS_TAG_NAME_MAXLEN.
				*vendor, // DVFS_DINODE_VENDOR_MAXLEN
				*description; // DVFS_DINODE_DESCRIPTION_MAXLEN
		struct
		{
			utf8Char	*name; // DVFS_TAG_NAME_MAXLEN
			ubit8		isDefault;
		} categories[4];

		ubit8		nCategories;
		ubit8		majorVersion, minorVersion;
		ubit16		patchVersion;
		void		(*entryAddress)(void);
		ubit32		flags;
	};

	/* A few typedefs for easier typename access.
	 **********************************************************************/
	class DistributaryInode;
	class categoryINode;

	class Tag
	:
	public vfs::Tag<DVFS_TAG_NAME_MAXLEN>
	{
	public:
		Tag(
			utf8Char *name,
			vfs::tagTypeE type,
			Tag *parent,
			vfs::Inode *inode=NULL)
		:
		vfs::Tag<DVFS_TAG_NAME_MAXLEN>(name, type, parent, inode)
		{}

		error_t initialize(void)
		{
			error_t			ret;

			ret = vfs::Tag<DVFS_TAG_NAME_MAXLEN>::initialize();
			if (ret != ERROR_SUCCESS) { return ret; };
			zkcmCore.chipsetEventNotification(
				__KPOWER_EVENT_DVFS_AVAIL, 0);

			return ERROR_SUCCESS;
		}

		~Tag(void) {}

	public:
		DistributaryInode *getDInode(void)
			{ return (DistributaryInode *)getInode(); }

		categoryINode *getCInode(void)
			{ return (categoryINode *)getInode(); }
	};

	/**	DistributaryInode:
	 * The metadata structure used to store information about distributaries
	 * available for execution by the kernel. The metadata is implicitly
	 * also a VFS tree of named objects so that userspace (and kernelspace)
	 * can easily query the status of distributaries, and manage them.
	 **********************************************************************/
	class DistributaryInode
	:
	public vfs::Inode
	{
	public:
		// Can be either in the kernel image, or loaded from elsewhere.
		enum typeE { IN_KERNEL=1, OUT_OF_KERNEL };

		DistributaryInode(
			const sDistributaryDescriptor *descriptor, typeE type);

		error_t initialize(void)
		{
			return vfs::Inode::initialize();
		}

		~DistributaryInode(void) {}

	public:
		sarch_t isCurrentlyRunning(void) { return currentlyRunning; }
		typeE getType(void) { return type; }
		void (*getEntryAddress(void))(void) { return entryAddress; }
		void getVersion(ubit8 *major, ubit8 *minor, ubit16 *patch);

	private:
		typeE			type;
		sarch_t			currentlyRunning;

		// Mirror members for sDistributaryDescriptor.
		utf8Char	name[DVFS_TAG_NAME_MAXLEN],
				vendor[DVFS_DINODE_VENDOR_MAXLEN],
				description[DVFS_DINODE_DESCRIPTION_MAXLEN];
		ubit8		majorVersion, minorVersion;
		ubit16		patchVersion;
		ubit32		flags;

		void		(*entryAddress)(void);
	};

	/**	categoryINode:
	 * Categories are really directories, but they can be "executed" or
	 * can act like leaf nodes when a caller does not specify one of their
	 * leaf nodes. More specifically, they hold a pointer to one of their
	 * leaf nodes which is taken to be the "default" distributary for the
	 * category in question.
	 *
	 * E.g:	"@d:/network" is a distributary category node.
	 *	"@d:/network/aqueductt" is a distributary node.
	 *
	 * Asking the kernel to start up the distributary @d:/network/aqueductt
	 * would start up Aqueductt. However, if aqueductt has been set as the
	 * "default" network service provider, asking the kernel to start up
	 * the "network" provider would work as well, and can be done by asking
	 * it to start "@d:/network", which will fall-through to Aqueductt.
	 **********************************************************************/
	class categoryINode
	:
	public vfs::DirInode<Tag>
	{
	public:
		categoryINode(void) {}
		error_t initialize(void)
			{ return vfs::DirInode<Tag>::initialize(); }

		~categoryINode(void) {}
	};

	/* The Currentt derived type for the DVFS currentt. Essentially
	 * represents the whole of the Distributary VFS tree.
	 **/
	class Currentt
	:
	public vfs::Currentt
	{
	friend class DistributaryTrib;
	public:
		Currentt(void)
		:
		vfs::Currentt(static_cast<utf8Char>( 'd' )),
		rootTag(
			CC"Zambesii Distributary VFS", vfs::DIR,
			&rootTag, &rootCategory),
		Tagache(NULL)
		{}

		error_t initialize(void)
		{
			error_t		ret;

			ret = rootTag.initialize();
			if (ret != ERROR_SUCCESS) { return ret; };
			return rootCategory.initialize();
		}

		~Currentt(void) {}

	public:
		Tag *getRoot(void) { return &rootTag; }

		// Both categories and dtribs can be returned.
		error_t getPath(utf8Char *fullName, Tag **ret);

	private:
		Tag			rootTag;
		categoryINode		rootCategory;
		SlamCache		*Tagache;

		struct sState
		{
			sState(void)
			:
			nDistributaries(0), nCategories(0)
			{}

			~sState(void) {}

			ubit16		nDistributaries, nCategories;
		};

		SharedResourceGroup<WaitLock, sState>	state;
	};
}

#endif

