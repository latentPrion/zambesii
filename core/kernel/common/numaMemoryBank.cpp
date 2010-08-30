
#include <debug.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <kernel/common/numaMemoryBank.h>


numaMemoryBankC::numaMemoryBankC(void)
{
	ranges.rsrc.arr = __KNULL;
	ranges.rsrc.nRanges = 0;
}

error_t numaMemoryBankC::initialize(void *preAllocated)
{
	return memBmp.initialize(preAllocated);
}

numaMemoryBankC::~numaMemoryBankC(void)
{
}

status_t numaMemoryBankC::addMemoryRange(
	paddr_t baseAddr, paddr_t size, void *mem
	)
{
	memBmpC		*bmp, **tmp;
	error_t		err;
	uarch_t		rwFlags, nRanges;

	// Allocate a new bmp allocator.
	bmp = new memBmpC(baseAddr, size);
	if (bmp == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};

	if (mem != __KNULL) {
		err = bmp->initialize(mem);
	}
	else {
		err = bmp->initialize();
	};

	if (err != ERROR_SUCCESS) {
		return static_cast<status_t>( err );
	};

	ranges.lock.writeAcquire();

	nRanges = ranges.rsrc.nRanges;
	tmp = new memBmpC*[nRanges + 1];
	if (tmp == __KNULL)
	{
		ranges.lock.writeRelease();
		delete bmp;
		return ERROR_MEMORY_NOMEM;
	};

	memcpy(tmp, ranges.rsrc.arr, sizeof(memBmpC *) * nRanges);
	delete ranges.rsrc.arr;
	ranges.rsrc.arr = tmp;
	ranges.rsrc.arr[nRanges] = bmp;
	ranges.rsrc.nRanges++;

	ranges.lock.writeRelease();

	return ERROR_SUCCESS;
}

status_t numaMemoryBankC::removeMemoryRange(paddr_t baseAddr)
{
	memBmpC		*tmp=0;

	ranges.lock.writeAcquire();

	for (uarch_t i=0; i<ranges.rsrc.nRanges; i++)
	{
		if ((baseAddr >= ranges.rsrc.arr[i]->baseAddr)
			&& (baseAddr <= ranges.rsrc.arr[i]->endAddr))
		{
			tmp = ranges.rsrc.arr[i];
			// Move every other pointer up to cover it.
			for (uarch_t j=i; j<(ranges.rsrc.nRanges - 1); j++) {
				ranges.rsrc.arr[j] = ranges.rsrc.arr[j+1];
			};

			ranges.rsrc.nRanges--;
			break;
		};
	};

	ranges.lock.writeRelease();

	// Memory range with this base address/contained address doesn't exist.
	if (tmp == __KNULL) {
		return ERROR_INVALID_ARG_VAL;
	};

	delete tmp;
}

error_t numaMemoryBankC::contiguousGetFrames(uarch_t nPages, paddr_t *paddr)
{
}

status_t numaMemoryBankC::fragmentedGetFrames(uarch_t nPages, paddr_t *paddr)
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

