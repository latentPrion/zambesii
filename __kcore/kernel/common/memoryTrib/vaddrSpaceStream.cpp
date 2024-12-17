
#include <debug.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/process.h>
#include <kernel/common/memoryTrib/vaddrSpaceStream.h>


error_t VaddrSpaceStream::initialize(void)
{
	error_t		ret;

	ret = vaddrSpace.initialize(parent->addrSpaceBinding);
	if (ret != ERROR_SUCCESS) { return ret; };
	ret = pageCache.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };
	return vSwamp.initialize();
}

error_t VaddrSpaceStream::bind(void)
{
	return ERROR_SUCCESS;
}

void VaddrSpaceStream::cut(void)
{
}

void VaddrSpaceStream::dump(void)
{
	printf(NOTICE"VaddrSpaceStream %x: Level0: v: %p, p: %P\n",
		id, vaddrSpace.level0Accessor.rsrc,
		&vaddrSpace.level0Paddr);

	printf(NOTICE"vaddrSpace object: v %p, p %P\n",
		vaddrSpace.level0Accessor.rsrc, &vaddrSpace.level0Paddr);

	vSwamp.dump();
}

void *VaddrSpaceStream::getPages(uarch_t nPages)
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

void VaddrSpaceStream::releasePages(void *vaddr, uarch_t nPages)
{
	// First try to free to the page cache.
	if (pageCache.push(nPages, vaddr) == ERROR_SUCCESS) {
		return;
	};

	// Cache free failed. Cache is full. Free to the swamp.
	vSwamp.releasePages(vaddr, nPages);
}

