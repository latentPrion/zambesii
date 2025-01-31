#ifndef _NUMA_MEMORY_BANK_H
	#define _NUMA_MEMORY_BANK_H

	#include <arch/paddr_t.h>
	#include <__kclasses/memBmp.h>
	#include <__kclasses/slamCache.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/multipleReaderLock.h>
	#include <kernel/common/numaMemoryRange.h>
	#include <kernel/common/numaTypes.h>

/**	EXPLANATION:
 * Just as each process has its own NUMA configuration, and a 'default' bank,
 * each NUMA Bank has its own bank-local configuration. There can be any number
 * of disjoint, allocatable memory ranges in a single NUMA memory bank. That is,
 * within NUMA bank 0, there could be N completely physically discontiguous
 * usable memory ranges, even going so far as to be able to have ranges of
 * memory for *different banks* all mixed up together. So a single NUMA bank
 * isn't necessarily physically a single memory range.
 *
 * Zambesii handles this messy problem by allowing multiple physical memory
 * allocation bitmaps to exist on each NUMA Memory bank. There is another way
 * to do this: Use a single bitmap object per NUMA bank, and make it as large
 * as the whole physical span of memory that covers all memory ranges that the
 * node owns, then mark all frames for all intermediate ranges of memory as
 * used. But this would make allocation on that bank very slow whenever all the
 * memory at the front of the BMP was allocated, and you then have to skip over
 * probably thousands of bits to get to the next range of bits that actually
 * pertain to this memory bank.
 *
 * Using multiple bitmaps per bank, one for each disjoint memory range makes
 * allocation faster and also allows us to support hot-plug of memory by simply
 * allocating/freeing a bitmap object.
 *
 * Each bitmap comes with its own frame cache, which speeds up allocations by
 * storing frees of common frame sizes.
 **/
#define NUMAMEMBANK_FLAGS_NO_AUTO_ALLOC_BMP	(1<<0)
#define NUMAMEMBANK			"Numa Memory Bank "

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

class NumaMemoryBank
{
public:
	NumaMemoryBank(numaBankId_t id);
	error_t initialize(void) { return rangePtrCache.initialize(); }
	~NumaMemoryBank(void);

public:
	// Adds __kspace to the memory ranges on a memory bank.
	error_t __kspaceAddMemoryRange(
		void *arrayMem,
		NumaMemoryRange *__kspace, void *__kspaceInitMem);

	error_t addMemoryRange(paddr_t baseAddr, paddr_t size);
	error_t removeMemoryRange(paddr_t baseAddr);


	void dump(void);

	void cut(void);
	void bind(void);

public:
	status_t constrainedGetFrames(
		fplainn::dma::constraints::Compiler *compiledConstraints,
		uarch_t nFrames,
		fplainn::dma::ScatterGatherList *retlist,
		ubit32 flags=0);

	status_t fragmentedGetFrames(
		uarch_t nFrames, paddr_t *paddr, ubit32 flags=0);

	void releaseFrames(paddr_t paddr, uarch_t nFrames);

	// Is a wrapper around NumaMemoryRange::mapMemU*sed().
	void mapMemUsed(paddr_t basePaddr, uarch_t nFrames);
	void mapMemUnused(paddr_t basePaddr, uarch_t nFrames);

	status_t merge(NumaMemoryBank *nmb);

	sarch_t identifyPaddr(paddr_t paddr);
	sarch_t identifyPaddrRange(paddr_t base, paddr_t nBytes);

public:
	struct sRangePtr
	{
		NumaMemoryRange	*range;
		sRangePtr		*next;
	};

	numaBankId_t	id;

private:
	SharedResourceGroup<MultipleReaderLock, sRangePtr *>	ranges;
	SharedResourceGroup<MultipleReaderLock, NumaMemoryRange *> defRange;
	SlamCache	rangePtrCache;
};

#endif

