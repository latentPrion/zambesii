
#include <kernel/common/numaMemoryBank.h>

error_t numaMemoryBankC::contiguousGetFrames(uarch_t nPages, paddr_t *paddr)
{
	if (frameCache.pop(nPages, paddr) == ERROR_SUCCESS) {
		return ERROR_SUCCESS;
	};

	// Frame cache allocation failed.
	return memBmp.contiguousGetFrames(nPages, paddr);
}

error_t numaMemoryBankC::fragmentedGetFrames(uarch_t nPages, paddr_t *paddr)
{
	uarch_t			minPages;

	// Try to see how much of the frame cache we can exhaust.
	minPages = frameCache.stacks[STACKCACHE_NSTACKS - 1].stackSize;

	// Determine which stack to start querying from.
	if (nPages < minPages) {
		minPages = nPages;
	};

	// Probably not as fast as it coule be, but faster than the BMP.
	for (; minPages > 0; minPages--)
	{
		if (frameCache.pop(minPages, paddr) == ERROR_SUCCESS) {
			return minPages;
		};
	};

	// Return whatever we get.
	return memBmp.fragmentedGetFrames(nPages, paddr);
}

void numaMemoryBankC::releaseFrames(paddr_t paddr, uarch_t nPages)
{
	// Attempt to free to the frame cache.
	if (frameCache.push(nPages, paddr) == ERROR_SUCCESS) {
		return;
	};

	// Bmp free.
	memBmp.releaseFrames(paddr, nPages);
}

// Couyld probably inline these two.
void numaMemoryBankC::mapRangeUsed(paddr_t baseAddr, uarch_t nFrames)
{
	memBmp.mapRangeUsed(baseAddr, nFrames);
}

void numaMemoryBankC::mapRangeUnused(paddr_t baseAddr, uarch_t nFrames)
{
	memBmp.mapRangeUnused(baseAddr, nFrames);
}

