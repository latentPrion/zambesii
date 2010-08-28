#ifndef _NUMA_MEMORY_BANK_H
	#define _NUMA_MEMORY_BANK_H

	#include <arch/paddr_t.h>
	#include <__kclasses/stackCache.h>
	#include <__kclasses/memBmp.h>

/**	EXPLANATION:
 * A NUMA memory Bank is an allocation API for Zambezii's Physical Memory
 * allocation abstraction.
 *
 * It is the base unit of physical memory allocation, and is not to be probed
 * any further. No class is to attempt to for example, reference and call its
 * internal BMP directly.
 *
 * The class numaMemoryBankC all physical memory allocation functions for both
 * contiguous and fragmented allocation.
 **/

#define NUMAMEMBANK_FLAGS_NO_AUTO_ALLOC_BMP	(1<<0)

class numaMemoryBankC
{
public:
	numaMemoryBankC(paddr_t baseAddr, paddr_t size);
	error_t initialize(void *preAllocated=__KNULL);
	~numaMemoryBankC(void);

	void cut(void);
	void bind(void);

public:
	// Returns physically contiguous RAM.
	error_t contiguousGetFrames(uarch_t nFrames, paddr_t *paddr);
	// Returns as many physically contiguous frames as it can find < nPages.
	error_t fragmentedGetFrames(uarch_t nFrames, paddr_t *paddr);
	void releaseFrames(paddr_t paddr, uarch_t nFrames);

	// Is a wrapper around memBmpC::mapRangeU*sed().
	void mapRangeUsed(paddr_t basePaddr, uarch_t nFrames);
	void mapRangeUnused(paddr_t basePaddr, uarch_t nFrames);

public:
	paddr_t			baseAddr, size;

private:
	memBmpC			memBmp;
	// FIXME: Have a look at this.
	stackCacheC<paddr_t>	frameCache;
};

#endif

