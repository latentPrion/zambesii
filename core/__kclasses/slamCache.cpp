
#include <debug.h>
#include <arch/paging.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/slamCache.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/memoryTrib/memoryTrib.h>

slamCacheC::slamCacheC(void)
{
}

slamCacheC::slamCacheC(uarch_t objectSize)
{
	initialize(objectSize);
}

error_t slamCacheC::initialize(uarch_t objectSize)
{
	heapCacheC::objectSize = ((objectSize < sizeof(slamCacheC::object))
		? sizeof(slamCacheC::object) : objectSize);

	freeList.rsrc = reinterpret_cast<object *>(
		(memoryTrib.__kmemoryStream.*
			memoryTrib.__kmemoryStream.memAlloc)(1, 0));

	if (freeList.rsrc != __KNULL) {
		freeList.rsrc->next = __KNULL;
	};

	partialList.rsrc = __KNULL;

	// Calculate the excess on each page allocated.
	perPageExcess = PAGING_BASE_SIZE % objectSize;
	perPageBlocks = PAGING_BASE_SIZE / objectSize;
	// Don't bother to check for freeList == __KNULL.
	return ERROR_SUCCESS;
}

slamCacheC::~slamCacheC(void)
{
	/* I'm not even sure it's possible to destroy a cache, really, without
	 * some form of reference counting.
	 **/
}

void slamCacheC::dump(void)
{
	slamCacheC::object	*obj;
	uarch_t			count;

	__kprintf(NOTICE SLAMCACHE"Dumping.\n");
	__kprintf(NOTICE SLAMCACHE"Object size: %X, ppb %d, ppexcess %d, "
		"FreeList: Pages:\n\t",
		objectSize, perPageBlocks, perPageExcess);

	count = 0;

	freeList.lock.acquire();

	for (obj = freeList.rsrc; obj != __KNULL; obj = obj->next, count++) {
		__kprintf((utf8Char *)"v %X ", obj);
	};

	freeList.lock.release();

	__kprintf((utf8Char*)"\n\t%d in total.\nPartialList: free objects:\n\t",
		count);

	count = 0;

	partialList.lock.acquire();

	for (obj = partialList.rsrc; obj != __KNULL; obj = obj->next, count++) {
		__kprintf((utf8Char *)"v %X, ", obj);
	};
	
	partialList.lock.release();

	__kprintf((utf8Char *)"\n\t%d in all.\n", count);
}

/**	NOTE:
 * Called by an idle thread to lock the heap and scan for free pages However,
 * this function will not cause the pages which were restored to be free()d to
 * the kernel memory stream. They will just remain on the free list.
 **/
status_t slamCacheC::detangle(void)
{
	return 0;
}

status_t slamCacheC::flush(void)
{
	slamCacheC::object	*current, *tmp;
	uarch_t			ret=0;

	freeList.lock.acquire();

	current  = freeList.rsrc;
	while (current != __KNULL)
	{
		tmp = current;
		current = current->next;
		memoryTrib.__kmemoryStream.memFree(tmp);
		ret++;
	};

	freeList.lock.release();
	return ret;
}

void *slamCacheC::allocate(void)
{
	void			*ret;
	slamCacheC::object	*tmp=__KNULL;

	partialList.lock.acquire();

	ret = partialList.rsrc;
	if (ret == __KNULL)
	{
		freeList.lock.acquire();

		if (freeList.rsrc != __KNULL)
		{
			tmp = freeList.rsrc;
			freeList.rsrc = freeList.rsrc->next;
		};

		freeList.lock.release();

		if (tmp == __KNULL)
		{
			tmp = new ((memoryTrib.__kmemoryStream
				.*memoryTrib.__kmemoryStream.memAlloc)(1, 0))
				slamCacheC::object;

			if (tmp == __KNULL)
			{
				partialList.lock.release();
				return __KNULL;
			};
		};

		// Break up the new block from the free list.
		for (uarch_t i=0; i<perPageBlocks; i++) {
			tmp[i].next = &tmp[i+1];
		};
		tmp[perPageBlocks].next = 0;
		partialList.rsrc = tmp;
	};

	ret = partialList.rsrc;
	partialList.rsrc = partialList.rsrc->next;
	partialList.lock.release();
	return ret;
}

void slamCacheC::free(void *obj)
{
	if (obj == __KNULL) {
		return;
	};

	partialList.lock.acquire();

	static_cast<slamCacheC::object *>( obj )->next = partialList.rsrc;
	partialList.rsrc = static_cast<slamCacheC::object *>( obj );

	partialList.lock.release();
}

