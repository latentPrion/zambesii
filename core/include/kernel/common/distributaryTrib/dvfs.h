#ifndef _DISTRIBUTARY_VFS_H
	#define _DISTRIBUTARY_VFS_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kclib/string8.h>
	#include <kernel/common/vfsTrib/vfsTypes.h>

#define DVFS_TAG_NAME_MAX_NCHARS		(48)

#define DVFS_DINODE_VENDOR_MAX_NCHARS		(64)
#define DVFS_DINODE_VENDORCONTACT_MAX_NCHARS	(128)
#define DVFS_DINODE_DESCRIPTION_MAX_NCHARS	(256)

#define DVFS_DINODE_FLAGS_DEFAULT		DVFS_DESCRIPTOR_FLAGS_DEFAULT

class distributaryTribC;

namespace dvfs
{
	/**	EXPLANATION:
	 * Exported by every distributary packaged with the kernel. Used by
	 * the kernel to create the Distributary VFS.
	 **/
	struct distributaryDescriptorS
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
		utf8Char	*name, // DVFS_TAG_NAME_MAX_NCHARS.
				*vendor, // DVFS_DINODE_VENDOR_MAX_NCHARS
				*description; // DVFS_DINODE_DESCRIPTION_MAX_NCHARS
		struct
		{
			utf8Char	*name; // DVFS_TAG_NAME_MAX_NCHARS
			ubit8		isDefault;
		} categories[4];

		ubit8		nCategories;
		ubit8		majorVersion, minorVersion;
		ubit16		patchVersion;
		void		*entryAddress;
		ubit32		flags;
	};

	/* A few typedefs for easier typename access.
	 **/
	class distributaryInodeC;
	class categoryInodeC;
	typedef vfs::tagC<
		distributaryInodeC, categoryInodeC, DVFS_TAG_NAME_MAX_NCHARS>
		distributaryTagC;

	typedef vfs::tagC<
		categoryInodeC, categoryInodeC, DVFS_TAG_NAME_MAX_NCHARS>
		categoryTagC;

	/* The metadata structure used to store information about distributaries
	 * available for execution by the kernel. The metadata is implicitly
	 * also a VFS tree of named objects so that userspace (and kernelspace)
	 * can easily query the status of distributaries, and manage them.
	 **/
	class distributaryInodeC
	:
	public vfs::leafInodeC
	{
	public:
		// Can be either in the kernel image, or loaded from elsewhere.
		enum typeE { IN_KERNEL=1, OUT_OF_KERNEL };

		distributaryInodeC(const distributaryDescriptorS *descriptor);
		error_t initialize(void)
			{ return vfs::leafInodeC::initialize(); }

		~distributaryInodeC(void) {}

	public:
		sarch_t isCurrentlyRunning(void) { return currentlyRunning; }
		typeE getType(void) { return type; }
		void *getEntryAddress(void) { return entryAddress; }
		void getVersion(ubit8 *major, ubit8 *minor, ubit16 *patch);

	private:
		typeE		type;
		sarch_t		currentlyRunning;

		// Mirror members for distributaryDescriptorS.
		utf8Char	name[DVFS_TAG_NAME_MAX_NCHARS],
				vendor[DVFS_DINODE_VENDOR_MAX_NCHARS],
				description[DVFS_DINODE_DESCRIPTION_MAX_NCHARS];
		ubit8		majorVersion, minorVersion;
		ubit16		patchVersion;
		void		*entryAddress;
		ubit32		flags;
	};

	/* Categories are really directories, but they can be "executed" or
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
	 **/
	class categoryInodeC
	:
	public vfs::dirInodeC<
		categoryInodeC, distributaryInodeC, DVFS_TAG_NAME_MAX_NCHARS>
	{
	public:
		categoryInodeC(void) {}
		error_t initialize(void)
		{
			return vfs::dirInodeC<
				categoryInodeC, distributaryInodeC,
				DVFS_TAG_NAME_MAX_NCHARS>::initialize();
		}

		~categoryInodeC(void) {}
	};

	/* The currenttC derived type for the DVFS currentt. Essentially
	 * represents the whole of the Distributary VFS tree.
	 **/
	class currenttC
	:
	public vfs::currenttC
	{
	friend class distributaryTribC;
	public:
		currenttC(void)
		:
		vfs::currenttC(static_cast<utf8Char>( 'd' )),
		rootTag(CC"Zambesii Distributary VFS", &rootTag, &rootCategory),
		tagCache(__KNULL)
		{}

		error_t initialize(void)
		{
			error_t		ret;

			ret = rootTag.initialize();
			if (ret != ERROR_SUCCESS) { return ret; };
			return rootCategory.initialize();
		}

		~currenttC(void) {}

	public:
		dvfs::categoryTagC *getRoot(void) { return &rootTag; }

		// Both categories and dtribs can be returned.
		void *getPath(
			utf8Char *fullName, vfs::inodeTypeE *type,
			error_t *ret);

	private:
		dvfs::categoryTagC		rootTag;
		dvfs::categoryInodeC		rootCategory;
		slamCacheC			*tagCache;

		struct stateS
		{
			stateS(void)
			:
			nDistributaries(0), nCategories(0)
			{}

			~stateS(void) {}

			ubit16		nDistributaries, nCategories;
		};

		sharedResourceGroupC<waitLockC, stateS>	state;
	};
}

#endif

