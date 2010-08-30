
#include <kernel/common/numaMemoryRange.h>

numaMemoryRangeC::numaMemoryRangeC(paddr_t baseAddr, paddr_t size)
:
bmp(baseAddr, size), baseAddr(baseAddr), size(size)
{
}

error_t numaMemoryRangeC::initialize(void *mem)
{
	return bmp.initialize(mem);
}

numaMemoryRangeC::~numaMemoryRangeC(void)
{
}

error_t numaMemoryRangeC::contiguousGetFrames(uarch_t nFrames, paddr_t *paddr)
{
	// See if the cache has a loaded stack for this nFrames size.
	if (frameCache.pop(nFrames, paddr) == ERROR_SUCCESS) {
		return nFrames;
	};

	// Else allocate from bmp.
	return bmp.contiguousGetFrames(nFrames, paddr);
}

error_t numaMemoryRangeC::fragmentedGetFrames(uarch_t nFrames, paddr_t *paddr)
{
	// See if the cache can satisfy any part of this alloc.
	if (frameCache.pop(1, paddr) == ERROR_SUCCESS) {
		return 1;
	};

	// Else allocate directly from the BMP.
	return bmp.fragmentedGetFrames(nFrames, paddr);
}

error_t numaMemoryRangeC::mapMemUsed(paddr_t baseAddr, uarch_t nFrames)
{
	UNIMPLEMENTED("error_t numaMemoryRangeC::mapMemUsed(paddr_t, uarch_t)");
	return ERROR_UNIMPLEMENTED;
}

error_t numaMemoryRangeC::mapMemUnused(paddr_t baseAddr, uarch_t nFrames)
{
	UNIMPLEMENTED("error_t numaMemoryRangeC::mapMemUsed(paddr_t, uarch_t)");
	return ERROR_UNIMPLEMENTED;
}

