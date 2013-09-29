#ifndef _POOL_ALLOCATOR_H
	#define _POOL_ALLOCATOR_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/allocClass.h>
	#include <__kclasses/memoryPool.h>

/**	EXPLANATION:
 * The Zambesii Pool Allocator is a layer of extra abstraction over the memory
 * management in the kernel which removes all concern over the underlying
 * architecture's page size, allowing the kernel to neatly allocate memory of
 * sizes up to as large as the largest pool size.
 *
 * It is essentially a heap. The pool allocator defines pools of different sizes
 * which DO NOT cache objects. A pool is a large block of memory which not in
 * any way specialized to any specific allocation size. Pools within the pool
 * allocator have different max allocation sizes determined by the pool's pool
 * size.
 *
 * So a 2MB pool will be able to allocate a max size of just under 2MB. Object
 * caching is implemented as a layer on top of this. Pools are *NOT*
 * automatically created based on demand. The kernel must explicitly create a
 * pool size before pools of that maximum size can be used.
 *
 * Generally, the Zambesii memory reservoir (what people call a heap) is managed
 * by a wrapping class, memReservoirC. memReservoir automatically directs all
 * allocations < PAGING_BASE_SIZE to the object caches.
 *
 * All allocations larger than PAGING_BASE_SIZE are directed here. If this
 * allocator cannot satisfy the allocation from the general pool, it will pass
 * the allocation on to the kernel's Memory Stream for a page granular
 * allocation.
 *
 *	ABOUT PRIVATE POOL CREATION.
 * If a specific subsystem in the kernel is in need of a custom pool allocator
 * for an allocation size which is greater than the general pool, an API is
 * provided for creating pools of alternate sizes for custom use. These pools
 * will not be allocated from for any calls other than those which specifically
 * pass the ID of the pool.
 *
 * When the creator has no use for the pool, it would be very nice if they would
 * release it.
 **/
class poolAllocatorC
:
public allocClassC
{
public:
	poolAllocatorC(void);
	// initialize() will chain-call initialize() on the public pool.
	error_t initialize(uarch_t poolSize);
	~poolAllocatorC(void);

public:
	void *publicAllocate(uarch_t nBytes);
	void publicFree(void *mem);

	// Returns a handle to the created pool, NULL if it failed.
	void *createPool(uarch_t poolSize);
	void releasePool(void *poolHandle);

	// Allocate from a custom pool.
	void *privateAllocate(void *poolHandle, uarch_t nBytes);
	void privateFree(void *poolHandle, void *mem);

public:
	memoryPoolC	*publicPool;
	memoryPoolC	**privatePools;
};

#endif

