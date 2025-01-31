#ifndef _NUMA_MEMORY_RANGE_H
	#define _NUMA_MEMORY_RANGE_H

	#include <arch/paddr_t.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/memBmp.h>
	#include <__kclasses/stackCache.h>

/**	EXPLANATION:
 * A NUMA Memory range represents a range of physically contiguous memory which
 * may belong to any NUMA bank. Each numaMemoryRange is provided with its own
 * personal frame cache to allow for faster allocation of frames from that
 * range.
 **/
namespace fplainn
{
namespace dma
{
	class Constraints;
	class ScatterGatherList;

namespace constraints
{
	class Compiler;
}
}
}

class NumaMemoryBank;

class NumaMemoryRange
{
friend class NumaMemoryBank;

public:
	NumaMemoryRange(paddr_t baseAddr, paddr_t size);
	error_t initialize(void *initMem=NULL);
	~NumaMemoryRange(void);

	void dump(void);

public:
	status_t fragmentedGetFrames(
		uarch_t nFrames, paddr_t *paddr, ubit32 flags=0);

	status_t constrainedGetFrames(
		fplainn::dma::constraints::Compiler *compiledConstraints,
		uarch_t nFrames,
		fplainn::dma::ScatterGatherList *retlist,
		ubit32 flags)
	{
		// constrainedGetFrames should not consult the page cache.
		return bmp.constrainedGetFrames(
			compiledConstraints, nFrames, retlist, flags);
	}

	void releaseFrames(paddr_t frameAddr, uarch_t nFrames);

	error_t mapMemUsed(paddr_t baseAddr, uarch_t nFrames);
	error_t mapMemUnused(paddr_t baseAddr, uarch_t nFrames);

	status_t merge(NumaMemoryRange *nmr);

	sarch_t identifyPaddr(paddr_t paddr);
	sarch_t identifyPaddrRange(paddr_t baseAddr, paddr_t nBytes);

private:
	paddr_t			baseAddr, size;
	MemoryBmp			bmp;
	StackCache<paddr_t>	frameCache;
};

#endif

