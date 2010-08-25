
#include <arch/paging.h>
#include <chipset/memory.h>
#include <__kstdlib/__kmath.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <__kclasses/memReservoir.h>
#include <kernel/common/memoryTrib/memoryTrib.h>


memReservoirC::memReservoirC(void)
{
	__kbog = __KNULL;
	bogs.rsrc.ptrs = __KNULL;
	bogs.rsrc.nBogs = 0;
	caches.rsrc.ptrs = __KNULL;
	caches.rsrc.ptrs = 0;
}

error_t memReservoirC::initialize(void)
{
	__kbog = new ((memoryTrib.__kmemoryStream
		.*memoryTrib.__kmemoryStream.memAlloc)(1, 0))
			memoryBogC(CHIPSET_MEMORY___KBOG_SIZE);

	if (__kbog == __KNULL)
	{
		__kprintf(ERROR RESERVOIR"Unable to allocate a bog for "
			"general kernel use.\n");

		return ERROR_MEMORY_NOMEM;
	};

	__kbog->initialize();

	caches.rsrc.ptrs = new ((memoryTrib.__kmemoryStream
		.*memoryTrib.__kmemoryStream.memAlloc)(1, 0)) slamCacheC*;

	memset(caches.rsrc.ptrs, 0, PAGING_BASE_SIZE);

	if (caches.rsrc.ptrs == __KNULL)
	{
		__kprintf(ERROR RESERVOIR"Unable to allocate a page to hold "
			"the array of object caches.\n");

		return ERROR_MEMORY_NOMEM;
	};

	bogs.rsrc.ptrs = new ((memoryTrib.__kmemoryStream
		.*memoryTrib.__kmemoryStream.memAlloc)(1, 0)) memoryBogC*;

	memset(bogs.rsrc.ptrs, 0, PAGING_BASE_SIZE);

	if (bogs.rsrc.ptrs == __KNULL)
	{
		__kprintf(ERROR RESERVOIR"Unable to allocate a page to hold "
			"the array of custom bog allocators.\n");

		return ERROR_MEMORY_NOMEM;
	};

	return ERROR_SUCCESS;
}

memReservoirC::~memReservoirC(void)
{
}

void memReservoirC::dump(void)
{
	
	__kprintf(NOTICE RESERVOIR"Dumping.\n");
	__kprintf(NOTICE RESERVOIR"Cache array, %X, nCaches %d, __kbog %X, "
		"(custom) bogs array %X, nBogs %d.\n",
		caches.rsrc.ptrs, caches.rsrc.nCaches, __kbog, bogs.rsrc.ptrs,
		bogs.rsrc.nBogs);

	__kprintf(NOTICE RESERVOIR"Dumping __kbog.\n");
	__kbog->dump();

	for (uarch_t i=0; i<bogs.rsrc.nBogs; i++)
	{
		__kprintf(NOTICE RESERVOIR"Dumping bog %d @ v %X.\n",
			i, bogs.rsrc.ptrs[i]);

		bogs.rsrc.ptrs[i]->dump();
	};

	for (uarch_t i=0; i<caches.rsrc.nCaches; i++)
	{
		__kprintf(NOTICE RESERVOIR"Dumping cache %d @ v %X.\n",
			i, caches.rsrc.ptrs[i]);

		caches.rsrc.ptrs[i]->dump();
	};
}

void *memReservoirC::allocate(uarch_t nBytes, uarch_t flags)
{
	uarch_t			rwFlags;
	reservoirHeaderS	*ret;
	slamCacheC		*cache;

	if (nBytes == 0) {
		return __KNULL;
	};

	if (nBytes + sizeof(reservoirHeaderS) < PAGING_BASE_SIZE / 2)
	{
		caches.lock.readAcquire(&rwFlags);

		if (caches.rsrc.ptrs != __KNULL)
		{
			for (uarch_t i=0; i<caches.rsrc.nCaches; i++)
			{
				if (caches.rsrc.ptrs[i]->objectSize
					== nBytes + sizeof(reservoirHeaderS))
				{
					ret = static_cast<reservoirHeaderS *>(
						caches.rsrc.ptrs[i]
							->allocate() );

					caches.lock.readRelease(rwFlags);

					ret->owner = caches.rsrc.ptrs[i];
					return reinterpret_cast<void *>(
						reinterpret_cast<uarch_t>( ret )
							+ sizeof(reservoirHeaderS) );
				};
			};

			caches.lock.readRelease(rwFlags);
			// Make sure cache doesn't exist in createCache();
			caches.lock.writeAcquire();

			// No cache. Try to allocate one.
			cache = createCache(nBytes + sizeof(reservoirHeaderS));

			caches.lock.writeRelease();
			caches.lock.readAcquire(&rwFlags);

			if (cache != __KNULL)
			{
				ret = static_cast<reservoirHeaderS *>(
					cache->allocate() );

				caches.lock.readRelease(rwFlags);

				if (ret != __KNULL)
				{
					ret->owner = cache;
					return reinterpret_cast<void *>(
						reinterpret_cast<uarch_t>( ret )
							+ sizeof(reservoirHeaderS) );
				};
			};
		};
		caches.lock.readRelease(rwFlags);
	};

	// Unable to allocate from an object cache. Do bog alloc.
	if (__kbog == __KNULL) {
		return __KNULL;
	};

	ret = reinterpret_cast<reservoirHeaderS *>(
		__kbog->allocate(nBytes, flags) );

	if (ret != __KNULL)
	{
		ret->owner = __KNULL;
		ret = reinterpret_cast<reservoirHeaderS *>(
			reinterpret_cast<uarch_t>( ret )
				+ sizeof(reservoirHeaderS) );
	};

	return ret;
}

void memReservoirC::free(void *_mem)
{
	reservoirHeaderS	*mem;
	slamCacheC		*cache;

	if (_mem == __KNULL) {
		return;
	};

	mem = reinterpret_cast<reservoirHeaderS *>(
		reinterpret_cast<uarch_t>( _mem ) - sizeof(reservoirHeaderS) );

	if (mem->owner != __KNULL)
	{
		cache = mem->owner;
		cache->free(mem);
		return;
	};

	// Not a cache allocated object. Free to bog.
	if (__kbog == __KNULL) {
		return;
	};
	__kbog->free(mem);
}

// Expects the lock to be pre-write-acquired.
slamCacheC *memReservoirC::createCache(uarch_t objSize)
{
	slamCacheC	*ret=0;
	uarch_t		spot=RESERVOIR_MAX_NCACHES;

	// It's possible that another thread allocated the cache between locks.
	for (uarch_t i=0; i<caches.rsrc.nCaches; i++)
	{
		if (caches.rsrc.ptrs[i]->objectSize == objSize) {
			return caches.rsrc.ptrs[i];
		};
	};

	if (__kbog != __KNULL) {
		ret = new (__kbog->allocate(sizeof(slamCacheC)))
			slamCacheC(objSize);
	};

	// Just do a page granular alloc.
	if (ret == __KNULL)
	{
		ret = new ((memoryTrib.__kmemoryStream
			.*memoryTrib.__kmemoryStream.memAlloc)(1, 0))
				slamCacheC(objSize);
	};

	if (ret == __KNULL) {
		return __KNULL;
	};

	for (uarch_t i=0; i<caches.rsrc.nCaches; i++)
	{
		if (objSize < caches.rsrc.ptrs[i]->objectSize)
		{
			spot = i;
			break;
		};
	};

	for (uarch_t i=caches.rsrc.nCaches; i > spot; i--) {
		caches.rsrc.ptrs[i] = caches.rsrc.ptrs[i-1];
	};
	caches.rsrc.ptrs[spot] = ret;
	caches.rsrc.nCaches++;
	return ret;
}

