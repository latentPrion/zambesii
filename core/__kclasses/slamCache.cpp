
#include <arch/paging.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/slamCache.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/memoryTrib/memoryTrib.h>


slamCacheC	*asyncContextCache;

slamCacheC::slamCacheC(uarch_t objectSize, allocatorE allocator)
:
allocator(allocator)
{
	heapCacheC::objectSize = (objectSize < sizeof(objectS))
		? sizeof(objectS)
		: objectSize;

	// Calculate the excess on each page allocated.
	perPageExcess = PAGING_BASE_SIZE % objectSize;
	perPageBlocks = PAGING_BASE_SIZE / objectSize;

	partialList.rsrc = NULL;
	freeList.rsrc = NULL;
}

slamCacheC::~slamCacheC(void)
{
	//FIXME: Find a way to destroy these things...
}

void *slamCacheC::getNewPage(sarch_t localFlush)
{
	switch (allocator)
	{
	case RAW:
		return memoryTrib.rawMemAlloc(
			1,
			MEMALLOC_NO_FAKEMAP
			| ((localFlush) ? MEMALLOC_LOCAL_FLUSH_ONLY : 0));

	case STREAM:
		return processTrib.__kgetStream()->memoryStream.memAlloc(
			1,
			MEMALLOC_NO_FAKEMAP
			| ((localFlush) ? MEMALLOC_LOCAL_FLUSH_ONLY : 0));

	default:
		printf(FATAL SLAMCACHE"getNewPage: Invalid allocator %d.\n",
			allocator);

		dump();
		panic(ERROR_INVALID_ARG);
		return NULL;
	};
}

void slamCacheC::releasePage(void *vaddr)
{
	switch (allocator)
	{
	case RAW:
		memoryTrib.rawMemFree(vaddr, 1);
		return;

	case STREAM:
		processTrib.__kgetStream()->memoryStream.memFree(vaddr);
		return;

	default:
		printf(FATAL SLAMCACHE"releasePage: Invalid allocator %d.\n",
			allocator);

		dump();
		panic(ERROR_INVALID_ARG);
		return;
	};
}

void slamCacheC::dump(void)
{
	objectS		*obj;
	uarch_t		count;

	printf(NOTICE SLAMCACHE"Dumping; locks @ F: 0x%p/P: 0x%p.\n",
		&freeList.lock, &partialList.lock);

	printf(NOTICE SLAMCACHE"Object size: %X, ppb %d, ppexcess %d, "
		"FreeList: Pages:\n\t",
		objectSize, perPageBlocks, perPageExcess);

	count = 0;

	freeList.lock.acquire();

	for (obj = freeList.rsrc; obj != NULL; obj = obj->next, count++) {
		printf((utf8Char *)"v %X ", obj);
	};

	freeList.lock.release();

	printf((utf8Char*)"\n\t%d in total.\nPartialList: free objects:\n\t",
		count);

	count = 0;

	partialList.lock.acquire();

	for (obj = partialList.rsrc; obj != NULL; obj = obj->next, count++) {
		printf((utf8Char *)"v %X, ", obj);
	};

	partialList.lock.release();

	printf((utf8Char *)"\n\t%d in all.\n", count);
}

/**	NOTE:
 * Called by an idle thread to lock the heap and scan for free pages. However,
 * this function will not cause the pages which were restored to be free()d to
 * the kernel memory stream. They will just remain on the free list.
 **/
status_t slamCacheC::detangle(void)
{
	return 0;
}

status_t slamCacheC::flush(void)
{
	slamCacheC::objectS	*current, *tmp;
	uarch_t			ret=0;

	freeList.lock.acquire();

	current  = freeList.rsrc;
	while (current != NULL)
	{
		tmp = current;
		current = current->next;
		releasePage(tmp);
		ret++;
	};

	freeList.lock.release();
	return ret;
}

void *slamCacheC::allocate(uarch_t flags, ubit8 *requiredNewPage)
{
	void			*ret;
	objectS			*tmp=NULL;
	sarch_t			localFlush;

	localFlush = FLAG_TEST(flags, SLAMCACHE_ALLOC_LOCAL_FLUSH_ONLY);

	partialList.lock.acquire();

	ret = partialList.rsrc;
	if (ret == NULL)
	{
		freeList.lock.acquire();

		if (freeList.rsrc != NULL)
		{
			tmp = freeList.rsrc;
			freeList.rsrc = freeList.rsrc->next;
		};

		freeList.lock.release();

		if (tmp == NULL)
		{
			if (requiredNewPage != NULL) {
				*requiredNewPage = 1;
			};

			tmp = new (getNewPage(localFlush)) objectS;

			if (tmp == NULL)
			{
				partialList.lock.release();
				return NULL;
			};
		};

		partialList.rsrc = tmp;

		// Break up the new block from the free list.
		for (uarch_t i=perPageBlocks-1; i>0; i--)
		{
			tmp->next = reinterpret_cast<objectS *>(
				(uintptr_t)tmp + this->objectSize );

			tmp = tmp->next;
		};
		tmp->next = NULL;
	};

	ret = partialList.rsrc;
	partialList.rsrc = partialList.rsrc->next;
	partialList.lock.release();
	return ret;
}

void slamCacheC::free(void *obj)
{
	if (obj == NULL) {
		return;
	};

	partialList.lock.acquire();

	static_cast<objectS *>( obj )->next = partialList.rsrc;
	partialList.rsrc = static_cast<objectS *>( obj );

	partialList.lock.release();
}

