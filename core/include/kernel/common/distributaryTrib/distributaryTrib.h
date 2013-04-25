#ifndef _DISTRIBUTARY_TRIB_H
	#define _DISTRIBUTARY_TRIB_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/currentt.h>
	#include <__kclasses/ptrList.h>
	#include <kernel/common/tributary.h>

	/**	EXPLANATION:
	 * The Distributary Tributary is the tributary responsible for managing
	 * the loading of kernel Distributaries. "Distributary" is the
	 * nomenclature used for a runtime loadable separate address space
	 * kernel component; many other kernels would call this a "service".
	 *
	 * The Distributary Trib provides a "VFS" of easily accessible named
	 * objects that refer to distributaries that were built into the kernel
	 * image.
	 **/

#define DTRIBTRIB		"Dtrib Trib: "

#define DTRIBTRIB_TAG_NAME_MAX_NCHARS		(48)

class slamCacheC;

class distributaryTribC
:
public tributaryC
{
public:
	distributaryTribC(void)
	:
	rootTag(CC"Distributary VFS"), dtribTagCache(__KNULL)
	{}

	// Builds the Dtrib VFS tree of compiled-in kernel distributaries.
	error_t initialize(void);
	~distributaryTribC(void) {};

public:
	class distributaryInodeC;
	class categoryInodeC;
	template <class inodeType> class dvfsTagC;
	typedef dvfsTagC<distributaryInodeC>	distributaryTagC;
	typedef dvfsTagC<categoryInodeC>	categoryTagC;

	categoryInodeC *getRootCategory(void) { return &rootCategory; }

public:
	/**	EXPLANATION:
	 * Exported by every distributary packaged with the kernel. Used by
	 * the kernel to create the Distributary VFS.
	 **/
	struct distributaryDescriptorS
	{
		utf8Char	*category,
				*name,
				*description,
				*vendor;
		ubit8		majorVersion, minorVersion;
		ubit16		patchVersion;
		void		*entryAddress;
	};

	/* VFS tag-node type. Typedefs are defined above to avoid the need
	 * for constant template argument specification.
	 **/	
	template <class inodeType>
	class dvfsTagC
	:
	public currentt::vfsTagC<DTRIBTRIB_TAG_NAME_MAX_NCHARS>
	{
	public:
		dvfsTagC(utf8Char *name, inodeType *inode=__KNULL)
		:
		currentt::vfsTagC<DTRIBTRIB_TAG_NAME_MAX_NCHARS>(name),
		inode(inode)
		{}

		error_t initialize(void)
		{
			return currentt::vfsTagC<DTRIBTRIB_TAG_NAME_MAX_NCHARS>
				::initialize();
		};

		~dvfsTagC(void) {}

	public:
		inodeType *getInode(void) { return inode; }
		void setInode(inodeType *inode) { this->inode = inode; }

	private:
		inodeType	*inode;
	};

	/* VFS Category Inode data-type. "Category" is really just a sort of
	 * folder used in the dtrib VFS.
	 **/
	class categoryInodeC
	:
	public currentt::vfsInodeC
	{
	public:
		categoryInodeC(void)
		:
		defaultDistributary(__KNULL)
		{};

		error_t initialize(void);
		~categoryInodeC(void) {};

	public:
		void setDefaultDistributary(distributaryTagC *defaultDtrib)
			{ defaultDistributary = defaultDtrib; }

		distributaryTagC *getDefaultDistributary(void)
			{ return defaultDistributary; }

		void dump(void)
		{
			categories.dump();
			distributaries.dump();
		}

		/* Functions for manipulating distributary tags.
		 **/
		distributaryTagC *createDistributaryTag(
			utf8Char *name, distributaryInodeC *inode=__KNULL);

		sarch_t removeDistributaryTag(utf8Char *name);
		sarch_t removeDistributaryTag(distributaryTagC *inode);
		distributaryTagC *getDistributaryTag(utf8Char *name);

		/* Functions for manipulating category tags.
		 **/
		categoryTagC *createCategory(utf8Char *name);
		sarch_t removeCategory(utf8Char *name);
		sarch_t removeCategory(categoryTagC *inode);
		categoryTagC *getCategory(utf8Char *name);

	private:
		ptrListC<categoryTagC>		categories;
		ptrListC<distributaryTagC>	distributaries;
		distributaryTagC		*defaultDistributary;
	};

	/* Dtrib leaf-level VFS node. Describes a distributary directly. Exactly
	 * one exists for each distributary that is compiled into the kernel.
	 * To avoid any mismatch between the layouts of this class and the
	 * distributaryDescriptorS structure, we just cause this class to
	 * inherit directly from the distributaryDescriptorS structure.
	 **/
	class distributaryInodeC
	:
	public currentt::vfsInodeC, protected distributaryDescriptorS
	{
	public:
		distributaryInodeC(distributaryDescriptorS &desc);

		error_t initialize(void)
			{ return currentt::vfsInodeC::initialize(); }

	public:
		void *getEntryAddress(void) { return entryAddress; }
	};

private:
	// Called at boot to construct the distributary VFS tree.
	void buildTree(void);

private:
	categoryTagC			rootTag;
	categoryInodeC			rootCategory;
	slamCacheC			*dtribTagCache;
	static const distributaryDescriptorS
		*const distributaryDescriptors[];
};

extern distributaryTribC	distributaryTrib;

#endif

