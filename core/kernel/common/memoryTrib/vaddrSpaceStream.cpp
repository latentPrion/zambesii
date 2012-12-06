
#include <debug.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/memoryTrib/vaddrSpaceStream.h>


//Constructor for userspace process streams.
vaddrSpaceStreamC::vaddrSpaceStreamC(
	uarch_t id,
	void *swampStart, uarch_t swampSize, 
	vSwampC::holeMapS *holeMap,
	pagingLevel0S *level0Accessor, paddr_t level0Paddr
	)
:
streamC(id),
vaddrSpace(level0Accessor, level0Paddr), vSwamp(swampStart, swampSize, holeMap)
{
	bind();
}

//Constructor for the kernel stream.
vaddrSpaceStreamC::vaddrSpaceStreamC(uarch_t id,
	pagingLevel0S *level0Accessor, paddr_t level0Paddr)
:
streamC(id),
vaddrSpace(level0Accessor, level0Paddr)
{
	bind();
}

sarch_t vaddrSpaceStreamC::initialize(
	void *swampStart, uarch_t swampSize, vSwampC::holeMapS *holeMap
	)
{
	return vSwamp.initialize(swampStart, swampSize, holeMap);
}

void vaddrSpaceStreamC::bind(void)
{
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

