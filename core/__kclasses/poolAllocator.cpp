
#include <arch/paging.h>
#include <__kstdlib/__kmath.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/poolAllocator.h>
#include <__kclasses/debugPipe.h>


poolAllocatorC::poolAllocatorC(void)
{
}

error_t initialize(void)
{
	return ERROR_SUCCESS;
}

void *poolAllocatorC::allocate(uarch_t size)
{
	uarch_t		realSize;
	slamCacheC	tmp, *cache;
	allocHeaderS	mem;

	// For now, only support allocations < PAGING_BASE_SIZE.
	if (size > PAGING_BASE_SIZE || size == 0) {
		return __KNULL;
	};

	/**	EXPLANATION:
	 * The idea is to see whether or not the cache size already exists.
	 * If it does, then allocate from there.
	 *
	 * Generally, you want to calculate the size of the header to attach
	 * to the block of memory and allocate that much, then put the header
	 * info into the memory block. Conventional wisdom would tell you that
	 * it's wise to place a magic number between the allocation size and
	 * the actual usable memory.
	 **/
	// All allocations are rounded up to nearest native word size boundary.
	__KMATH_P2ROUND_UP_BY(size, sizeof(uarch_t));
	realSize = size + ALLOCHEADER_SIZE;
	tmp.objectSize = realSize;

	cache = caches.find(&tmp);
	if (cache == __KNULL)
	{
		// Object size hasn't been allocated yet. Create new cache.
		cache = caches.addEntry(&tmp);
		if (cache == __KNULL) {
			return __KNULL;
		};

		// Construct the object.
		cache = new (cache) slamCacheC(realSize);
	};

	// As of now, 'cache' points to an object cache for 'realSize'.
	mem = static_cast<slamCacheC::allocHeaderS *>( cache->allocate() );
	if (mem == __KNULL) {
		return __KNULL;
	};

	mem->size = realSize;
	mem->magic = ALLOCHEADER_MAGIC;
	return &mem->firstByte;
}

void poolAllocatorC::free(void *_mem)
{
	allocHeaderS	*mem;
	slamCacheC	*cache, tmp;

	if (_mem == __KNULL) {
		return;
	};

	mem = reinterpret_cast<allocHeaderS *>(
		reinterpret_cast<uarch_t>( _mem ) - ALLOCHEADER_SIZE );

	if (mem->magic != ALLOCHEADER_MAGIC)
	{
		__kprintf(WARNING"Pool Allocator: Corrupt memory or bad free "
			"at %X. Size: %d, Magic: %X.\n",
			mem, mem->size, mem->magic);

		return;
	};

	// If all the checks passed, just find the cache and free.
	tmp.objectSize = mem->size;
	cache = caches.find(&tmp);

	if (cache == __KNULL)
	{
		__kprintf(ERROR"Pool Allocator: Mem header passed, but cache "
			"not found. Mem: %X, realSize: %d.\n",
			mem, mem->size);

		return;
	};

	cache->free(mem);
}

