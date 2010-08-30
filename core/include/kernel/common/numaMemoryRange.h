#ifndef _NUMA_MEMORY_RANGE_H
	#define _NUMA_MEMORY_RANGE_H

	#include <arch/paddr_t.h>
	#include <__kstdlib/__ktypes.h>

/**	EXPLANATION:
 * A NUMA Memory range represents a range of physically contiguous memory which
 * may belong to any NUMA bank. Each numaMemoryRange is provided with its own
 * personal frame cache to allow for faster allocation of frames from that
 * range.
 **/
class numaMemoryRangeC
{
public:
	numaMemoryRangeC(paddr_t baseAddr, paddr_t size);
	error_t initialize(void *mem=__KNULL);
	~numaMemoryRangeC(void);

public:
	error_t contiguousGetFrames(uarch_t nFrames, paddr_t *paddr);
	status_t fragmentedGetFrames(uarch_t nFrames, paddr_t *paddr);
	void releaseFrames(paddr_t frameAddr, uarch_t nFrames);

	error_t mapMemUsed(paddr_t baseAddr, uarch_t nFrames);
	error_t mapMemUnused(paddr_t baseAddr, uarch_t nFrames);

private:
	memBmpC			bmp;
	stackCacheC<paddr_t>	frameCache;
};

#endif

