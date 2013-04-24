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

#define DTRIBTRIB		"dtribTrib: "

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

	categoryInodeC *getRootCategory(void) { return &rootCategory; }

public:
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

		~dvfsTagC(void) {}

		inodeType *getInode(void) { return inode; }
		void setInode(inodeType *inode) { this->inode = inode; }

	private:
		inodeType	*inode;
	};

	class categoryInodeC
	:
	public currentt::vfsInodeC
	{
	public:
		categoryInodeC(void)
		:
		defaultDistributary(__KNULL)
		{};

		error_t initialize(void)
		{
			error_t		ret;

			ret = distributaries.initialize();
			if (ret != ERROR_SUCCESS) { return ret; };

			return categories.initialize();
		};

		~categoryInodeC(void) {};

		void setDefaultDistributary(
			dvfsTagC<distributaryInodeC> *defaultDtrib)
		{
			defaultDistributary = defaultDtrib;
		}

		void dump(void)
		{
			categories.dump();
			distributaries.dump();
		}

		dvfsTagC<distributaryInodeC> *getDefaultDistributary(void)
			{ return defaultDistributary; }

		/* Functions for manipulating distributary tags.
		 **/
		dvfsTagC<distributaryInodeC> *createDistributaryTag(
			utf8Char *name, distributaryInodeC *inode=__KNULL);

		sarch_t removeDistributaryTag(utf8Char *name);
		sarch_t removeDistributaryTag(
			dvfsTagC<distributaryInodeC> *inode);

		dvfsTagC<distributaryInodeC> *getDistributaryTag(utf8Char *name);

		/* Functions for manipulating category tags.
		 **/
		dvfsTagC<categoryInodeC> *createCategory(utf8Char *name);
		sarch_t removeCategory(utf8Char *name);
		sarch_t removeCategory(
			dvfsTagC<categoryInodeC> *inode);

		dvfsTagC<categoryInodeC> *getCategory(utf8Char *name);

		ptrListC< dvfsTagC<categoryInodeC> >		categories;
	private:
		ptrListC< dvfsTagC<distributaryInodeC> >	distributaries;
		dvfsTagC<distributaryInodeC>		*defaultDistributary;
	};

	class distributaryInodeC
	:
	public currentt::vfsInodeC
	{
		distributaryInodeC(void *entryPoint)
		:
		entryPoint(entryPoint)
		{}

		error_t initialize(void) { return ERROR_SUCCESS; }

		void *getEntryPoint(void);

	private:
		void		*entryPoint;
	};

private:
	void buildTree(void);

private:
	dvfsTagC<categoryInodeC>	rootTag;
	categoryInodeC			rootCategory;
	slamCacheC			*dtribTagCache;
};

extern distributaryTribC	distributaryTrib;

#endif

