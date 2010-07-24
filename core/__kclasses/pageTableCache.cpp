
#include <__kclasses/pageTableCache.h>
#include <kernel/common/numaTrib/numaTrib.h>

pageTableCacheC::pageTableCacheC(void)
{
	stackPtr.rsrc = PTCACHE_STACK_EMPTY;
}

pageTableCacheC::~pageTableCacheC(void)
{
}

void pageTableCacheC::push(paddr_t paddr)
{
	stackPtr.lock.acquire();

	if (stackPtr.rsrc == PTCACHE_STACK_FULL)
	{
		stackPtr.lock.release();
		// Free to the NUMA Tributary. Cache is full.
		numaTrib.releaseFrames(paddr, 1);
		return;
	}

	// There's space. Increment pointer then write.
	stackPtr.rsrc++;
	stack[stackPtr.rsrc] = paddr;

	stackPtr.lock.release();
}

error_t pageTableCacheC::pop(paddr_t *paddr)
{
	stackPtr.lock.acquire();

	if (stackPtr.rsrc == PTCACHE_STACK_EMPTY)
	{
		stackPtr.lock.release();
		// Allocate a new frame from the NUMA Tributary.
		return ((numaTrib.fragmentedGetFrames(1, paddr) > 0)
			? ERROR_SUCCESS : ERROR_MEMORY_NOMEM_PHYSICAL);
	};

	// There are frames in the cache. Pop the top and return it.
	*paddr = stack[stackPtr.rsrc];
	stackPtr.rsrc--;

	stackPtr.lock.release();
	return ERROR_SUCCESS;
}

void pageTableCacheC::flush(void)
{
	stackPtr.lock.acquire();

	// Flush all frames in the cache. Used when low on pmem.
	for (; stackPtr.rsrc != PTCACHE_STACK_EMPTY; stackPtr.rsrc--) {
		numaTrib.releaseFrames(stack[stackPtr.rsrc], 1);
	};

	stackPtr.lock.release();
}

