#ifndef _MEM_BMP_H
	#define _MEM_BMP_H

	#include <__kstdlib/__kbitManipulation.h>
	#include <__kstdlib/__kclib/assert.h>
	#include <__kclasses/allocClass.h>
	#include <kernel/common/waitLock.h>
	#include <kernel/common/panic.h>
	#include <kernel/common/sharedResourceGroup.h>

/**	EXPLANATION:
 * Simple bitmap, 1/0 for used/free. Will soon be changed into some sort of
 * highly efficient doubled layered setup. This combined with the frame caching
 * that accompanies each bitmap in its NUMA Bank wrapper will make most frame
 * allocations *very* fast.
 **/

#define MEMBMP				"MemBMP: "

#define MEMBMP_FLAGS_DYNAMIC		(1<<0)

class NumaMemoryBank;

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

class MemoryBmp
:
public AllocatorBase
{
friend class NumaMemoryBank;

public:
	MemoryBmp(paddr_t baseAddr, paddr_t size);
	error_t initialize(void *preAllocated=NULL);

	~MemoryBmp(void);

	void dump(void);

public:
	/* Searches the bitmap for frames that match the compiled constraints
	 * specified in "compiledConstraints".
	 */
	status_t constrainedGetFrames(
		fplainn::dma::constraints::Compiler *compiledConstraints,
		uarch_t nFrames,
		fplainn::dma::ScatterGatherList *retlist,
		ubit32 flags=0);

	status_t fragmentedGetFrames(uarch_t nFrames, paddr_t *paddr);
	sbit8 releaseFrames(paddr_t frameAddr, uarch_t nFrames);

	void mapMemUsed(paddr_t basePaddr, uarch_t nFrames);
	void mapMemUnused(paddr_t basePaddr, uarch_t nFrames);

	/* Does a big OR operation on this bmp, such that "this" |= bmp.
	 * Useful for the PMM, when transitioning from __kspace into NUMA Trib
	 * detected memory.
	 *
	 * Returns the number of bits that were set in "this" bmp due to the OR
	 * with the other one.
	 **/
	status_t merge(MemoryBmp *bmp);

public:
	inline void setFrame(uarch_t pfn);
	inline void unsetFrame(uarch_t pfn);
	inline sbit8 testFrame(uarch_t pfn);

public:
	uarch_t		basePfn, endPfn, bmpNFrames, flags, nIndexes;
	uarch_t		bmpSize;
	paddr_t		baseAddr, endAddr;

	struct sBmpState
	{
		uarch_t		*bmp, lastAllocIndex;
	};
	SharedResourceGroup<WaitLock, sBmpState>	bmp;
};


/**	Inline Methods
 ******************************************************************************/

inline void MemoryBmp::setFrame(uarch_t pfn)
{
	assert_fatal(pfn >= basePfn && pfn <= endPfn);

	setBit(bmp.rsrc.bmp, pfn - basePfn);
}

inline void MemoryBmp::unsetFrame(uarch_t pfn)
{
	assert_fatal(pfn >= basePfn && pfn <= endPfn);

	unsetBit(bmp.rsrc.bmp, pfn - basePfn);
}

inline sbit8 MemoryBmp::testFrame(uarch_t pfn)
{
	assert_fatal(pfn >= basePfn && pfn <= endPfn);

	return bitIsSet(bmp.rsrc.bmp, pfn - basePfn);
}

#endif

