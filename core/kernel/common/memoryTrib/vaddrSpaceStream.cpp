
#include <debug.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/memoryTrib/vaddrSpaceStream.h>


error_t vaddrSpaceStreamC::bind(void)
{
	return ERROR_SUCCESS;
}

void vaddrSpaceStreamC::cut(void)
{
}

void vaddrSpaceStreamC::dump(void)
{
	__kprintf(NOTICE"VaddrSpaceStream %X: Level0: v: %p, p: %p\n",
		id, vaddrSpace.level0Accessor.rsrc,
		vaddrSpace.level0Paddr);

	__kprintf(NOTICE"vaddrSpace object: v %X, p %X\n",
		vaddrSpace.level0Accessor.rsrc, vaddrSpace.level0Paddr);

	vSwamp.dump();
}

void *vaddrSpaceStreamC::getPages(uarch_t nPages)
{
	void		*ret = 0;

	// First try to allocate from the page cache.
	if (pageCache.pop(nPages, &ret) == ERROR_SUCCESS) {
		return ret;
	};

	/* Cache allocation attempt failed. Get free pages from the swamp. If that
	 * fails then it means the process simply has no more virtual memory.
	 **/
	return vSwamp.getPages(nPages);
}

void vaddrSpaceStreamC::releasePages(void *vaddr, uarch_t nPages)
{
	// First try to free to the page cache.
	if (pageCache.push(nPages, vaddr) == ERROR_SUCCESS) {
		return;
	};

	// Cache free failed. Cache is full. Free to the swamp.
	vSwamp.releasePages(vaddr, nPages);
}

