
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/cachePool.h>
#include <kernel/common/distributaryTrib/distributaryTrib.h>


distributaryTribC::dvfsTagC<distributaryTribC::distributaryInodeC> *
distributaryTribC::categoryInodeC::createDistributaryTag(
	utf8Char *name, distributaryInodeC *inode
	)
{
	distributaryTagC	*newTag;
	categoryTagC		*tmpCat;

	if (name == __KNULL) { return __KNULL; };

	// First check to see if it exists already:
	newTag = getDistributaryTag(name);
	if (newTag != __KNULL) { return newTag; };
	tmpCat = getCategory(name);
	if (tmpCat != __KNULL) { return __KNULL; };

	newTag = new (distributaryTrib.dtribTagCache->allocate())
		distributaryTagC(name, inode);

	if (newTag == __KNULL) { return __KNULL; };
	// Add the tag to the list.
	if (distributaries.insert(newTag) != ERROR_SUCCESS) { return __KNULL; };
	return newTag;
}

sarch_t distributaryTribC::categoryInodeC::removeDistributaryTag(utf8Char *name)
{
	void			*handle;
	distributaryTagC	*curr;

	if (name == __KNULL) { return 0; };

	distributaries.lock();

	handle = __KNULL;
	// Pass NO_AUTOLOCK because we've explicitly locked the list ourselves.
	curr = distributaries.getNextItem(&handle, PTRLIST_FLAGS_NO_AUTOLOCK);
	for (; curr != __KNULL;
		curr = distributaries.getNextItem(
			&handle, PTRLIST_FLAGS_NO_AUTOLOCK))
	{
		if (strncmp8(
			curr->getName(), name, DTRIBTRIB_TAG_NAME_MAX_NCHARS)
			!= 0)
		{
			continue;
		};

		// Found it.
		distributaries.unlock();

		distributaries.remove(curr);
		return 1;
	};

	distributaries.unlock();
	return 0;
}

sarch_t distributaryTribC::categoryInodeC::removeDistributaryTag(
	distributaryTagC *inode
	)
{
	if (inode == __KNULL) { return 0; };

	return distributaries.remove(inode);
}

distributaryTribC::dvfsTagC<distributaryTribC::distributaryInodeC> *
distributaryTribC::categoryInodeC::getDistributaryTag(utf8Char *name)
{
	distributaryTagC	*curr;
	void			*handle;

	if (name == __KNULL) { return __KNULL; };

	distributaries.lock();

	handle = __KNULL;
	curr = distributaries.getNextItem(&handle, PTRLIST_FLAGS_NO_AUTOLOCK);
	for (; curr != __KNULL;
		curr = distributaries.getNextItem(
			&handle, PTRLIST_FLAGS_NO_AUTOLOCK))
	{
		if (strncmp8(
			curr->getName(), name, DTRIBTRIB_TAG_NAME_MAX_NCHARS)
				!= 0)
		{
			continue;
		};

		// Found it.
		distributaries.unlock();
		return curr;
	};

	distributaries.unlock();
	return __KNULL;
}

distributaryTribC::dvfsTagC<distributaryTribC::categoryInodeC> *
distributaryTribC::categoryInodeC::createCategory(utf8Char *name)
{
	categoryTagC		*newTag;
	distributaryTagC	*tmpDtrib;

	if (name == __KNULL) { return __KNULL; };

	// First check to see if it already exists.
	newTag = getCategory(name);
	if (newTag != __KNULL) { return newTag; };
	tmpDtrib = getDistributaryTag(name);
	if (tmpDtrib != __KNULL) { return __KNULL; };

	// Allocate the new tag.
	newTag = new (distributaryTrib.dtribTagCache->allocate())
		categoryTagC(name);

	if (newTag == __KNULL) { return __KNULL; };

	// Now allocate and initialize the inode to go with it.
	newTag->setInode(new categoryInodeC);
	if (newTag->getInode() == __KNULL)
	{
		delete newTag;
		return __KNULL;
	};

	if (newTag->getInode()->initialize() != ERROR_SUCCESS
		|| categories.insert(newTag) != ERROR_SUCCESS)
	{
		delete newTag->getInode();
		delete newTag;
		return __KNULL;
	};

	return newTag;
}

sarch_t distributaryTribC::categoryInodeC::removeCategory(utf8Char *name)
{
	void		*handle;
	categoryTagC	*curr;

	if (name == __KNULL) { return 0; };

	handle = __KNULL;

	categories.lock();

	curr = categories.getNextItem(&handle, PTRLIST_FLAGS_NO_AUTOLOCK);
	for (; curr != __KNULL;
		curr = categories.getNextItem(
			&handle, PTRLIST_FLAGS_NO_AUTOLOCK))
	{
		if (strncmp8(
			curr->getName(), name, DTRIBTRIB_TAG_NAME_MAX_NCHARS)
				!= 0)
		{
			continue;
		};

		// Found it.
		categories.unlock();
		return categories.remove(curr);
	};

	categories.unlock();
	return 0;
}

sarch_t distributaryTribC::categoryInodeC::removeCategory(
	categoryTagC *inode
	)
{
	if (inode == __KNULL) { return 0; };

	return categories.remove(inode);
}

distributaryTribC::dvfsTagC<distributaryTribC::categoryInodeC> *
distributaryTribC::categoryInodeC::getCategory(utf8Char *name)
{
	categoryTagC	*curr;
	void		*handle;

	if (name == __KNULL) { return __KNULL; };

	handle = __KNULL;

	categories.lock();

	curr = categories.getNextItem(&handle, PTRLIST_FLAGS_NO_AUTOLOCK);
	for (; curr != __KNULL; 
		curr = categories.getNextItem(
			&handle, PTRLIST_FLAGS_NO_AUTOLOCK))
	{
		if (strncmp8(
			curr->getName(), name, DTRIBTRIB_TAG_NAME_MAX_NCHARS)
				!= 0)
		{
			continue;
		};

		// Found it.
		categories.unlock();
		return curr;
	};

	categories.unlock();
	return __KNULL;
}


