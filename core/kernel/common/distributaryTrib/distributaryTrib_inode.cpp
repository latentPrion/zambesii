
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/cachePool.h>
#include <kernel/common/distributaryTrib/distributaryTrib.h>


error_t distributaryTribC::categoryInodeC::initialize(void)
{
	error_t		ret;

	ret = currentt::vfsInodeC::initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	ret = distributaries.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	return categories.initialize();
}

distributaryTribC::dvfsTagC<distributaryTribC::distributaryInodeC> *
distributaryTribC::categoryInodeC::createDistributaryTag(
	utf8Char *name, distributaryInodeC *inode
	)
{
	distributaryTagC	*newTag;
	categoryTagC		*tmpCat;

	if (name == NULL) { return NULL; };

	// First check to see if it exists already:
	newTag = getDistributaryTag(name);
	if (newTag != NULL) { return newTag; };
	tmpCat = getCategory(name);
	if (tmpCat != NULL) { return NULL; };

	newTag = new (distributaryTrib.dtribTagCache->allocate())
		distributaryTagC(name, inode);

	if (newTag == NULL) { return NULL; };
	// Add the tag to the list.
	if (distributaries.insert(newTag) != ERROR_SUCCESS) { return NULL; };
	return newTag;
}

sarch_t distributaryTribC::categoryInodeC::removeDistributaryTag(utf8Char *name)
{
	void			*handle;
	distributaryTagC	*curr;

	if (name == NULL) { return 0; };

	distributaries.lock();

	handle = NULL;
	// Pass NO_AUTOLOCK because we've explicitly locked the list ourselves.
	curr = distributaries.getNextItem(&handle, PTRLIST_FLAGS_NO_AUTOLOCK);
	for (; curr != NULL;
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
	if (inode == NULL) { return 0; };

	return distributaries.remove(inode);
}

distributaryTribC::dvfsTagC<distributaryTribC::distributaryInodeC> *
distributaryTribC::categoryInodeC::getDistributaryTag(utf8Char *name)
{
	distributaryTagC	*curr;
	void			*handle;

	if (name == NULL) { return NULL; };

	distributaries.lock();

	handle = NULL;
	curr = distributaries.getNextItem(&handle, PTRLIST_FLAGS_NO_AUTOLOCK);
	for (; curr != NULL;
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
	return NULL;
}

distributaryTribC::dvfsTagC<distributaryTribC::categoryInodeC> *
distributaryTribC::categoryInodeC::createCategory(utf8Char *name)
{
	categoryTagC		*newTag;
	distributaryTagC	*tmpDtrib;

	if (name == NULL) { return NULL; };

	// First check to see if it already exists.
	newTag = getCategory(name);
	if (newTag != NULL) { return newTag; };
	tmpDtrib = getDistributaryTag(name);
	if (tmpDtrib != NULL) { return NULL; };

	// Allocate the new tag.
	newTag = new (distributaryTrib.dtribTagCache->allocate())
		categoryTagC(name);

	if (newTag == NULL) { return NULL; };

	// Now allocate and initialize the inode to go with it.
	newTag->setInode(new categoryInodeC);
	if (newTag->getInode() == NULL)
	{
		delete newTag;
		return NULL;
	};

	if (newTag->getInode()->initialize() != ERROR_SUCCESS
		|| categories.insert(newTag) != ERROR_SUCCESS)
	{
		delete newTag->getInode();
		delete newTag;
		return NULL;
	};

	return newTag;
}

sarch_t distributaryTribC::categoryInodeC::removeCategory(utf8Char *name)
{
	void		*handle;
	categoryTagC	*curr;

	if (name == NULL) { return 0; };

	handle = NULL;

	categories.lock();

	curr = categories.getNextItem(&handle, PTRLIST_FLAGS_NO_AUTOLOCK);
	for (; curr != NULL;
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
	if (inode == NULL) { return 0; };

	return categories.remove(inode);
}

distributaryTribC::dvfsTagC<distributaryTribC::categoryInodeC> *
distributaryTribC::categoryInodeC::getCategory(utf8Char *name)
{
	categoryTagC	*curr;
	void		*handle;

	if (name == NULL) { return NULL; };

	handle = NULL;

	categories.lock();

	curr = categories.getNextItem(&handle, PTRLIST_FLAGS_NO_AUTOLOCK);
	for (; curr != NULL; 
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
	return NULL;
}


