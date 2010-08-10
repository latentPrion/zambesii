
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <kernel/common/numaMemoryBank.h>


/**	NOTES:
 * In order to support uniformity in the kernel for all physical memory
 * management, we have to have the internal BMP in the numaMemoryBank objects
 * be a pointer to a BMP, and not a BMP object instance.
 *
 * But for the general case, it is desirable for the class to auto-allocate
 * the internal BMP object automatically. On startup, however, for the initial
 * temporary NUMA bank (__kspace), we do not wish for the class to allocate
 * the internal BMP automatically in its constructor.
 *
 * Thus, the internal allocation is controlled by a flag argument to the
 * constructor. By default, the flags are set to have the class auto-allocate.
 *
 * During kernel orientation, the kernel passes the 'no-allocate' flag, so that
 * it can later send a message to the NUMA Tributary to manually allocate the
 * internal BMP on the first bank.
 **/
numaMemoryBankC::numaMemoryBankC(void)
{
}

numaMemoryBankC::numaMemoryBankC(
	paddr_t baseAddr, paddr_t size, void *preAllocated
	)
{
	initialize(baseAddr, size, preAllocated);
}

error_t numaMemoryBankC::initialize(
	paddr_t baseAddr, paddr_t size, void *preAllocated
	)
{
	error_t		ret;

	this->baseAddr = baseAddr;
	this->size = size;

	ret = memBmp.initialize(baseAddr, size, preAllocated);
	if (ret != ERROR_SUCCESS) {
		return ret;
	};

	return ERROR_SUCCESS;
}

numaMemoryBankC::~numaMemoryBankC(void)
{
}

error_t numaMemoryBankC::contiguousGetFrames(uarch_t nPages, paddr_t *paddr)
{
	if (frameCache.pop(nPages, paddr) == ERROR_SUCCESS)
	{
		// FIXME: Passing a paddr_t to printf is asking for trouble.
		__kprintf(NOTICE"numaMemoryBank: contiguousGetFrames(%d) "
			"returning %p from cache.\n", nPages, *paddr);

		return ERROR_SUCCESS;
	};

	// Frame cache allocation failed.
	__kprintf(NOTICE"numaMemoryBank: contiguousGetFrames(%d): BMP "
		"allocated, p: %p\n", nPages, *paddr);

	return memBmp.contiguousGetFrames(nPages, paddr);
}

status_t numaMemoryBankC::fragmentedGetFrames(uarch_t nPages, paddr_t *paddr)
{
	uarch_t			minPages;
	status_t		ret;

	// Try to see how much of the frame cache we can exhaust.
	minPages = frameCache.stacks[STACKCACHE_NSTACKS - 1].stackSize;

	// Determine which stack to start querying from.
	if (nPages < minPages) {
		minPages = nPages;
	};

	// Probably not as fast as it coule be, but faster than the BMP.
	for (; minPages > 0; minPages--)
	{
		if (frameCache.pop(minPages, paddr) == ERROR_SUCCESS)
		{
			__kprintf(NOTICE"numaMemoryBank: fragmentedGetFrames(%d) "
			"returning %d frames at %p from cache.\n", nPages, minPages, *paddr);

			return minPages;
		};
	};

	// Return whatever we get.
	ret = memBmp.fragmentedGetFrames(nPages, paddr);

	__kprintf(NOTICE"numaMemoryBank: fragmentedGetFrames(%d): BMP "
		"allocated, %d pages at p: %p\n", nPages, ret, *paddr);

	return ret;
}

void numaMemoryBankC::releaseFrames(paddr_t paddr, uarch_t nPages)
{
	// Attempt to free to the frame cache.
	if (frameCache.push(nPages, paddr) == ERROR_SUCCESS)
	{
		__kprintf(NOTICE"numaMemoryBank: releaseFrames(%p, %d): "
			"freeing to cache.\n", paddr, nPages);

		return;
	};

	// Bmp free.
	memBmp.releaseFrames(paddr, nPages);

	__kprintf(NOTICE"numaMemoryBank: releaseFrames(%p, %d): BMP "
		"free: done.\n", paddr, nPages);
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

