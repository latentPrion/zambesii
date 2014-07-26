
#include <arch/paging.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/numaMemoryRange.h>

numaMemoryRangeC::numaMemoryRangeC(paddr_t baseAddr, paddr_t size)
:
baseAddr(baseAddr), size(size), bmp(baseAddr, size)
{
}

error_t numaMemoryRangeC::initialize(void *initMem)
{
	return bmp.initialize(initMem);
}

void numaMemoryRangeC::dump(void)
{
	bmp.dump();
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

sarch_t numaMemoryRangeC::identifyPaddrRange(paddr_t paddr, paddr_t nBytes)
{
	paddr_t		callerEndPaddr;
	paddr_t		rangeEndPaddr;

	callerEndPaddr = paddr + nBytes - 1;
	rangeEndPaddr = baseAddr + size - 1;

	if (((paddr >= this->baseAddr) && (paddr < rangeEndPaddr))
		|| ((callerEndPaddr >= this->baseAddr)
		&& (callerEndPaddr <= rangeEndPaddr)))
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

error_t numaMemoryRangeC::contiguousGetFrames(
	uarch_t nFrames, paddr_t *paddr, ubit32
	)
{
	// See if the cache has a loaded stack for this nFrames size.
	if (frameCache.pop(nFrames, paddr) == ERROR_SUCCESS) {
		return nFrames;
	};

	// Else allocate from bmp.
	return bmp.contiguousGetFrames(nFrames, paddr);
}

error_t numaMemoryRangeC::fragmentedGetFrames(
	uarch_t nFrames, paddr_t *paddr, ubit32
	)
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
	/* For a numaMemoryRangeC, to map memory used is bit more complex than
	 * one would think. You need to flush all of the frames from the
	 * frame cache, otherwise there may be left-over frames from the
	 * reserved range in the cache that could be handed out to applications.
	 **/
	if (!identifyPaddrRange(baseAddr, nFrames << PAGING_BASE_SHIFT)) {
		return ERROR_INVALID_ARG_VAL;
	};

	frameCache.flush(&bmp);
	bmp.mapMemUsed(baseAddr, nFrames);
	return ERROR_SUCCESS;
}

error_t numaMemoryRangeC::mapMemUnused(paddr_t baseAddr, uarch_t nFrames)
{
	/* To map frames unused for a numaMemoryRange, you can just map, and
	 * not flush the frame cache.
	 **/
	if (!identifyPaddrRange(baseAddr, nFrames << PAGING_BASE_SHIFT)) {
		return ERROR_INVALID_ARG_VAL;
	};

	bmp.mapMemUnused(baseAddr, nFrames);
	return ERROR_SUCCESS;
}

status_t numaMemoryRangeC::merge(numaMemoryRangeC *nmr)
{
	frameCache.flush(&bmp);
	return bmp.merge(&nmr->bmp);
}
