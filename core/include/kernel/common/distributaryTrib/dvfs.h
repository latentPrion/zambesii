#ifndef _DISTRIBUTARY_VFS_H
	#define _DISTRIBUTARY_VFS_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kclib/string8.h>
	#include <kernel/common/vfsTrib/vfsTypes.h>
	#include <__kclasses/cachePool.h>

#define DVFS_TAG_NAME_MAX_NCHARS	(48)

namespace dvfs
{
	/**	EXPLANATION:
	 * Exported by every distributary packaged with the kernel. Used by
	 * the kernel to create the Distributary VFS.
	 **/
	struct distributaryDescriptorS
	{
		utf8Char	category[DVFS_TAG_NAME_MAX_NCHARS],
				name[DVFS_TAG_NAME_MAX_NCHARS],
				description[256],
				vendor[64];
		ubit8		majorVersion, minorVersion;
		ubit16		patchVersion;
		void		*entryAddress;
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
	public vfs::leafInodeC, protected distributaryDescriptorS
	{
	public:
		// Can be either in the kernel image, or loaded from elsewhere.
		enum typeE { IN_KERNEL=1, OUT_OF_KERNEL };

		distributaryInodeC(
			typeE type, distributaryDescriptorS *descriptor);

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
		categoryInodeC(void)
		{
			defaultDtribName[0] = '\0';
		}

		error_t initialize(void)
		{
			return vfs::dirInodeC<
				categoryInodeC, distributaryInodeC,
				DVFS_TAG_NAME_MAX_NCHARS>::initialize();
		}

		~categoryInodeC(void) {}

	public:
		error_t setDefault(utf8Char *name)
		{
			if (getLeafTag(name) == __KNULL)
				{ return ERROR_INVALID_ARG; };

			strncpy8(
				defaultDtribName, name,
				DVFS_TAG_NAME_MAX_NCHARS);

			return ERROR_SUCCESS;
		}

		distributaryTagC *getDefault(void)
			{ return getLeafTag(defaultDtribName); }

	private:
		utf8Char	defaultDtribName[DVFS_TAG_NAME_MAX_NCHARS];
	};

	/* The currenttC derived type for the DVFS currentt. Essentially
	 * represents the whole of the Distributary VFS tree.
	 **/
	class currenttC
	:
	public vfs::currenttC
	{
	public:
		currenttC(void)
		:
		vfs::currenttC(static_cast<utf8Char>( 'd' )),
		rootTag(CC"Zambesii Distributary VFS", &rootTag, &rootCategory),
		tagCache(__KNULL)
		{}

		error_t initialize(void);
		~currenttC(void) {}

	private:
		dvfs::categoryInodeC *getRootCategory(void)
			{ return &rootCategory; }

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

