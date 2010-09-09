#ifndef _NUMA_MEMORY_BANK_H
	#define _NUMA_MEMORY_BANK_H

	#include <arch/paddr_t.h>
	#include <__kclasses/memBmp.h>
	#include <__kclasses/slamCache.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/multipleReaderLock.h>
	#include <kernel/common/numaMemoryRange.h>

/**	EXPLANATION:
 * Just as each process has its own NUMA configuration, and a 'default' bank,
 * each NUMA Bank has its own bank-local configuration. There can be any number
 * of disjoint, allocatable memory ranges in a single NUMA memory bank. That is,
 * within NUMA bank 0, there could be N completely physically discontiguous
 * usable memory ranges, even going so far as to be able to have ranges of
 * memory for *different banks* all mixed up together. So a single NUMA bank
 * isn't necessarily physically a single memory range.
 *
 * Zambezii handles this messy problem by allowing multiple physical memory
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
#define NUMAMEMBANK			"Numa Memory Bank: "

class numaMemoryBankC
{
public:
	numaMemoryBankC(void);
	~numaMemoryBankC(void);

	// Adds __kspace to the memory ranges on a memory bank.
	error_t __kspaceAddMemoryRange(
		void *arrayMem,
		numaMemoryRangeC *__kspace, void *__kspaceInitMem);

	error_t addMemoryRange(paddr_t baseAddr, paddr_t size);
	error_t removeMemoryRange(paddr_t baseAddr);

	void dump(void);

	void cut(void);
	void bind(void);

public:
	error_t contiguousGetFrames(uarch_t nFrames, paddr_t *paddr);
	status_t fragmentedGetFrames(uarch_t nFrames, paddr_t *paddr);
	void releaseFrames(paddr_t paddr, uarch_t nFrames);

	// Is a wrapper around numaMemoryRangeC::mapMemU*sed().
	void mapMemUsed(paddr_t basePaddr, uarch_t nFrames);
	void mapMemUnused(paddr_t basePaddr, uarch_t nFrames);

	sarch_t identifyPaddr(paddr_t paddr);
	// sarch_t identifyPaddrRange(paddr_t base, uarch_t nFrames);

public:
	struct rangePtrS
	{
		numaMemoryRangeC	*range;
		rangePtrS		*next;
	};

private:
	sharedResourceGroupC<multipleReaderLockC, rangePtrS *>	ranges;
	sharedResourceGroupC<multipleReaderLockC, numaMemoryRangeC *> defRange;
	slamCacheC	rangePtrCache;
};

#endif

