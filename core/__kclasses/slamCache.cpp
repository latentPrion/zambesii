
#include <arch/paging.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/slamCache.h>
#include <__kclasses/debugPipe.h>
#include <__kclasses/cachePool.h>
#include <kernel/common/panic.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/memoryTrib/memoryTrib.h>


SlamCache	*__kcallbackCache;

SlamCache::SlamCache(uarch_t _sObjectize, allocatorE allocator)
:
allocator(allocator)
{
	sObjectize = (_sObjectize < sizeof(sObject))
		? sizeof(sObject)
		: _sObjectize;

	// Calculate the excess on each page allocated.
	perPageExcess = PAGING_BASE_SIZE % sObjectize;
	perPageBlocks = PAGING_BASE_SIZE / sObjectize;

	partialList.rsrc = NULL;
	freeList.rsrc = NULL;
}

SlamCache::~SlamCache(void)
{
	//FIXME: Find a way to destroy these things...
}

void *SlamCache::getNewPage(sarch_t localFlush)
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

void SlamCache::releasePage(void *vaddr)
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

void SlamCache::dump(void)
{
	sObject		*obj;
	uarch_t		count;

	printf(NOTICE SLAMCACHE"@0x%p: Dumping; locks @ F: 0x%p/P: 0x%p.\n",
		this, &freeList.lock, &partialList.lock);

	printf(NOTICE SLAMCACHE"@0x%p: Object size: %X, ppb %d, ppexcess %d, "
		"FreeList: Pages:\n\t",
		this, sObjectize, perPageBlocks, perPageExcess);

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
status_t SlamCache::detangle(void)
{
	return 0;
}

status_t SlamCache::flush(void)
{
	SlamCache::sObject	*current, *tmp;
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

void *SlamCache::allocate(uarch_t flags, ubit8 *requiredNewPage)
{
	void			*ret;
	sObject			*tmp=NULL;
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

			tmp = new (getNewPage(localFlush)) sObject;

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
			tmp->next = reinterpret_cast<sObject *>(
				(uintptr_t)tmp + this->sObjectize );

			tmp = tmp->next;
		};
		tmp->next = NULL;
	};

	ret = partialList.rsrc;
	partialList.rsrc = partialList.rsrc->next;
	partialList.lock.release();
	return ret;
}

void SlamCache::free(void *obj)
{
	if (obj == NULL) {
		return;
	};

	partialList.lock.acquire();

	static_cast<sObject *>( obj )->next = partialList.rsrc;
	partialList.rsrc = static_cast<sObject *>( obj );

	partialList.lock.release();
}

