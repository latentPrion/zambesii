
#include <arch/paging.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/numaMemoryRange.h>

numaMemoryRangeC::numaMemoryRangeC(paddr_t baseAddr, paddr_t size)
:
baseAddr(baseAddr), size(size), bmp(baseAddr, size)
{
}

error_t numaMemoryRangeC::initialize(void *mem)
{
	return bmp.initialize(mem);
}

numaMemoryRangeC::~numaMemoryRangeC(void)
{
}

sarch_t numaMemoryRangeC::identifyPaddr(paddr_t baseAddr)
{
	if ((baseAddr >= numaMemoryRangeC::baseAddr)
		&& baseAddr <= (numaMemoryRangeC::baseAddr + (size-1)))
	{
		return 1;
	};
	return 0;
}

sarch_t numaMemoryRangeC::identifyPaddrRange(paddr_t paddr, uarch_t nFrames)
{
	paddr_t		endPaddr;
	paddr_t		rangeEndPaddr;

	endPaddr = paddr + (nFrames * PAGING_BASE_SIZE) - 1;
	rangeEndPaddr = baseAddr + size - 1;

	if (((paddr >= this->baseAddr) && (paddr < rangeEndPaddr))
		|| ((endPaddr >= this->baseAddr) && (endPaddr < rangeEndPaddr)))
	{
		return 1;
	};
	return 0;
}

void numaMemoryRangeC::releaseFrames(paddr_t paddr, uarch_t nFrames)
{
	if (frameCache.push(nFrames, paddr) == ERROR_SUCCESS) {
		return;
	};

	// Else free to BMP.
	bmp.releaseFrames(paddr, nFrames);
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

error_t numaMemoryRangeC::mapMemUsed(paddr_t, uarch_t)
{
	UNIMPLEMENTED("error_t numaMemoryRangeC::mapMemUsed()");
	return ERROR_UNIMPLEMENTED;
}

error_t numaMemoryRangeC::mapMemUnused(paddr_t, uarch_t)
{
	UNIMPLEMENTED("error_t numaMemoryRangeC::mapMemUnused()");
	return ERROR_UNIMPLEMENTED;
}

