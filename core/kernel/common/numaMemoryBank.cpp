
#include <arch/paging.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <kernel/common/numaMemoryBank.h>
#include <kernel/common/processTrib/processTrib.h>


#define NUMAMEMBANK_DEFINDEX_NONE	(-1)

NumaMemoryBank::NumaMemoryBank(numaBankId_t id)
:
id(id), rangePtrCache(sizeof(NumaMemoryBank::sRangePtr))
{
	ranges.rsrc = NULL;
	defRange.rsrc = NULL;
}

NumaMemoryBank::~NumaMemoryBank(void)
{
	sRangePtr	*tmp;

	do
	{
		ranges.lock.writeAcquire();

		tmp = ranges.rsrc;
		if (tmp != NULL) {
			ranges.rsrc = ranges.rsrc->next;
		};

		ranges.lock.writeRelease();

		if (tmp == NULL) {
			return;
		};

		// Make sure we don't mess up the kernel by freeing __kspace.
		if (!(reinterpret_cast<uintptr_t>( tmp->range )
			& PAGING_BASE_MASK_LOW))
		{
			processTrib.__kgetStream()->memoryStream.memFree(
				tmp->range);
		};
		rangePtrCache.free(tmp);
	} while (1);
}

void NumaMemoryBank::dump(void)
{
	uarch_t		rwFlags, rwFlags2;

	ranges.lock.readAcquire(&rwFlags);
	defRange.lock.readAcquire(&rwFlags2);

	printf(NOTICE NUMAMEMBANK"%d: Dumping. Default range base %P.\n",
		id, &defRange.rsrc->baseAddr);

	defRange.lock.readRelease(rwFlags2);

	for (sRangePtr *cur = ranges.rsrc; cur != NULL; cur = cur->next)
	{
		printf((utf8Char *)"\tMem range: base %P, size %P.\n",
			&cur->range->baseAddr, &cur->range->size);

		cur->range->dump();
	};

	ranges.lock.readRelease(rwFlags);
}

error_t NumaMemoryBank::__kspaceAddMemoryRange(
	void *ptrNodeMem, NumaMemoryRange *__kspace, void *__kspaceInitMem
	)
{
	ranges.rsrc = static_cast<sRangePtr *>( ptrNodeMem );
	ranges.rsrc->range = __kspace;
	defRange.rsrc = __kspace;

	return ranges.rsrc->range->initialize(__kspaceInitMem);
}

error_t NumaMemoryBank::addMemoryRange(paddr_t baseAddr, paddr_t size)
{
	NumaMemoryRange	*memRange;
	sRangePtr		*tmpNode;
	error_t			err;

	// Allocate a new bmp allocator.
	memRange = new (processTrib.__kgetStream()->memoryStream.memAlloc(
		PAGING_BYTES_TO_PAGES(sizeof(NumaMemoryRange)),
		MEMALLOC_NO_FAKEMAP))
			NumaMemoryRange(baseAddr, size);

	if (memRange == NULL) {
		return ERROR_MEMORY_NOMEM;
	};

	err = memRange->initialize();
	if (err != ERROR_SUCCESS)
	{
		processTrib.__kgetStream()->memoryStream.memFree(memRange);
		return err;
	};

	tmpNode = new (rangePtrCache.allocate()) sRangePtr;
	if (tmpNode == NULL)
	{
		processTrib.__kgetStream()->memoryStream.memFree(memRange);
		return ERROR_MEMORY_NOMEM;
	};

	tmpNode->range = memRange;

	ranges.lock.writeAcquire();

	tmpNode->next = ranges.rsrc;
	ranges.rsrc = tmpNode;

	defRange.lock.writeAcquire();

	// If the bank had no ranges before we got here:
	if (defRange.rsrc == NULL) {
		defRange.rsrc = memRange;
	};

	defRange.lock.writeRelease();
	ranges.lock.writeRelease();

	printf(NOTICE NUMAMEMBANK"%d: New mem range: base %P, size %P, "
		"v %p.\n",
		id, &baseAddr, &size, memRange);

	return ERROR_SUCCESS;
}

error_t NumaMemoryBank::removeMemoryRange(paddr_t baseAddr)
{
	sRangePtr		*cur, *prev=NULL;

	ranges.lock.writeAcquire();

	for (cur = ranges.rsrc; cur != NULL; )
	{
		if (cur->range->identifyPaddr(baseAddr))
		{
			defRange.lock.writeAcquire();

			if (defRange.rsrc == cur->range) {
				defRange.rsrc = NULL;
			};

			defRange.lock.writeRelease();

			// If we're removing the first range in the list:
			if (ranges.rsrc == cur) {
				ranges.rsrc = cur->next;
			}
			else {
				prev->next = cur->next;
			};

			// Can release lock and free now.
			ranges.lock.writeRelease();

			printf(NOTICE NUMAMEMBANK"%d: Destroying mem range: "
				"base %P, size %P, v %p.\n",
				id, &cur->range->baseAddr, &cur->range->size,
				cur->range);

			// Make sure we don't mess up by freeing __kspace.
			if (!(reinterpret_cast<uintptr_t>( cur->range )
				& PAGING_BASE_MASK_LOW))
			{
				processTrib.__kgetStream()->memoryStream
					.memFree(cur->range);
			};
			rangePtrCache.free(cur);
			return ERROR_SUCCESS;
		};
		prev = cur;
		cur = cur->next;
	};

	ranges.lock.writeRelease();

	// Memory range with this base address/contained address doesn't exist.
	printf(NOTICE NUMAMEMBANK"%d: Failed to remove range with base %P."
		"\n", id, &baseAddr);

	return ERROR_INVALID_ARG_VAL;
}

error_t NumaMemoryBank::contiguousGetFrames(
	uarch_t nFrames, paddr_t *paddr, ubit32)
{
	uarch_t		rwFlags, rwFlags2;
	error_t		ret;

	defRange.lock.readAcquire(&rwFlags);

	if (defRange.rsrc == NULL)
	{
		// Check and see if any new ranges have been added recently.
		ranges.lock.readAcquire(&rwFlags2);

		if (ranges.rsrc == NULL)
		{
			// This bank has no associated ranges of memory.
			ranges.lock.readRelease(rwFlags2);
			defRange.lock.readRelease(rwFlags);
			return ERROR_UNKNOWN;
		};

		defRange.lock.readReleaseWriteAcquire(rwFlags);
		defRange.rsrc = ranges.rsrc->range;
		defRange.lock.writeRelease();

		ranges.lock.readRelease(rwFlags2);
		defRange.lock.readAcquire(&rwFlags);
		// Note that we still hold readAcquire on defRange here.
	};


	// Allocate from the default first.
	ret = defRange.rsrc->contiguousGetFrames(nFrames, paddr);
	if (ret == ERROR_SUCCESS)
	{
		defRange.lock.readRelease(rwFlags);
		return ret;
	};

	// Default has no mem. Below we'll scan all the other ranges.
	ranges.lock.readAcquire(&rwFlags2);

	// We now hold both locks.
	for (sRangePtr *cur = ranges.rsrc; cur != NULL; cur = cur->next)
	{
		// Don't waste time re-trying the same range.
		if (cur->range == defRange.rsrc) {
			continue;
		};

		ret = cur->range->contiguousGetFrames(nFrames, paddr);
		if (ret == ERROR_SUCCESS)
		{
			defRange.lock.readReleaseWriteAcquire(rwFlags);
			defRange.rsrc = cur->range;
			defRange.lock.writeRelease();
			ranges.lock.readRelease(rwFlags2);
			return ret;
		};
	};

	// Reaching here means no mem was found.
	defRange.lock.readRelease(rwFlags);
	ranges.lock.readRelease(rwFlags2);
	return ERROR_MEMORY_NOMEM_PHYSICAL;
}

status_t NumaMemoryBank::fragmentedGetFrames(
	uarch_t nFrames, paddr_t *paddr, ubit32)
{
	uarch_t		rwFlags, rwFlags2;
	error_t		ret;

	defRange.lock.readAcquire(&rwFlags);

	if (defRange.rsrc == NULL)
	{
		// Check and see if any new ranges have been added recently.
		ranges.lock.readAcquire(&rwFlags2);

		if (ranges.rsrc == NULL)
		{
			// This bank has no associated ranges of memory.
			ranges.lock.readRelease(rwFlags2);
			defRange.lock.readRelease(rwFlags);
			return ERROR_MEMORY_NOMEM_PHYSICAL;
		};

		defRange.lock.readReleaseWriteAcquire(rwFlags);
		defRange.rsrc = ranges.rsrc->range;
		defRange.lock.writeRelease();

		ranges.lock.readRelease(rwFlags2);
		defRange.lock.readAcquire(&rwFlags);
		// Note that we still hold readAcquire on defRange here.
	};

	// Allocate from the default first.
	ret = defRange.rsrc->fragmentedGetFrames(nFrames, paddr);
	if (ret > 0)
	{
		defRange.lock.readRelease(rwFlags);
		return ret;
	};

	// Default has no mem. Below we'll scan all the other ranges.
	ranges.lock.readAcquire(&rwFlags2);
	// We now hold both locks.
	for (sRangePtr *cur = ranges.rsrc; cur != NULL; cur = cur->next)
	{
		// Don't waste time re-trying the same range.
		if (cur->range == defRange.rsrc) {
			continue;
		};

		ret = cur->range->fragmentedGetFrames(nFrames, paddr);
		if (ret > 0)
		{
			ranges.lock.readRelease(rwFlags2);
			defRange.lock.readReleaseWriteAcquire(rwFlags);
			defRange.rsrc = cur->range;
			defRange.lock.writeRelease();
			return ret;
		};
	};

	// Reaching here means no mem was found.
	ranges.lock.readRelease(rwFlags2);
	defRange.lock.readRelease(rwFlags);

	return ERROR_MEMORY_NOMEM_PHYSICAL;
}

status_t NumaMemoryBank::constrainedGetFrames(
	fplainn::Zudi::dma::DmaConstraints::Compiler *constraints,
	uarch_t nFrames,
	fplainn::Zudi::dma::ScatterGatherList *retlist,
	ubit32 flags
	)
{
	status_t	ret;

	/**	EXPLANATION:
	 * We don't care about setting a new default range. We are attempting
	 * to get memory according to a constraints specification.
	 *
	 * If the allocation is successful, it doesn't mean we can use that
	 * fact to optimize future allocations.
	 **/
	for (sRangePtr *cur = ranges.rsrc; cur != NULL; cur = cur->next)
	{
		ret = cur->range->constrainedGetFrames(
			constraints, nFrames, retlist, flags);

		if (ret < (signed)nFrames) {
			continue;
		};

		return ret;
	}

	return ERROR_MEMORY_NOMEM_PHYSICAL;
}

void NumaMemoryBank::releaseFrames(paddr_t basePaddr, uarch_t nFrames)
{
	uarch_t		rwFlags;

	ranges.lock.readAcquire(&rwFlags);

	for (sRangePtr *cur = ranges.rsrc; cur != NULL; cur = cur->next)
	{
		if (cur->range->identifyPaddr(basePaddr))
		{
			cur->range->releaseFrames(basePaddr, nFrames);

			ranges.lock.readRelease(rwFlags);
			return;
		};
	};

	ranges.lock.readRelease(rwFlags);

	printf(WARNING NUMAMEMBANK"%d: releaseFrames(%P, %d) Pmem leak.\n",
		id, &basePaddr, nFrames);
}

sarch_t NumaMemoryBank::identifyPaddr(paddr_t paddr)
{
	uarch_t		rwFlags;

	ranges.lock.readAcquire(&rwFlags);

	for (sRangePtr *cur = ranges.rsrc; cur != NULL; cur = cur->next)
	{
		/* A paddr can only correspond to ONE memory range. We never
		 * hand out pmem allocations spanning multiple discontiguous
		 * physical memory ranges in one allocation. Therefore when
		 * freeing, it's impossible for us to get a pmem or pmem range
		 * to be freed which isn't contiguous, and within one range.
		 **/
		if (cur->range->identifyPaddr(paddr))
		{
			ranges.lock.readRelease(rwFlags);
			return 1;
		};
	};
	ranges.lock.readRelease(rwFlags);
	return 0;
}

sarch_t NumaMemoryBank::identifyPaddrRange(paddr_t basePaddr, paddr_t nBytes)
{
	uarch_t		rwFlags;

	ranges.lock.readAcquire(&rwFlags);

	for (sRangePtr *cur = ranges.rsrc; cur != NULL; cur = cur->next)
	{
		if (cur->range->identifyPaddrRange(basePaddr, nBytes))
		{
			ranges.lock.readRelease(rwFlags);
			return 1;
		};
	};
	ranges.lock.readRelease(rwFlags);
	return 0;
}

void NumaMemoryBank::mapMemUsed(paddr_t baseAddr, uarch_t nFrames)
{
	uarch_t		rwFlags;

	ranges.lock.readAcquire(&rwFlags);

	for (sRangePtr *cur = ranges.rsrc; cur != NULL; cur = cur->next) {
		cur->range->mapMemUsed(baseAddr, nFrames);
	};
	ranges.lock.readRelease(rwFlags);
}

void NumaMemoryBank::mapMemUnused(paddr_t baseAddr, uarch_t nFrames)
{
	uarch_t		rwFlags;

	ranges.lock.readAcquire(&rwFlags);

	for (sRangePtr *cur = ranges.rsrc; cur != NULL; cur = cur->next) {
		cur->range->mapMemUnused(baseAddr, nFrames);
	};

	ranges.lock.readRelease(rwFlags);
}

status_t NumaMemoryBank::merge(NumaMemoryBank *nmb)
{
	uarch_t		rwFlags, rwFlags2;
	status_t	ret=0;

	ranges.lock.readAcquire(&rwFlags);
	nmb->ranges.lock.readAcquire(&rwFlags2);

	for (sRangePtr *cur = ranges.rsrc; cur != NULL; cur = cur->next)
	{
		for (sRangePtr *cur2 = nmb->ranges.rsrc; cur2 != NULL;
			cur2 = cur2->next)
		{
			ret += cur->range->merge(cur2->range);
		};
	};

	nmb->ranges.lock.readRelease(rwFlags2);
	ranges.lock.readRelease(rwFlags);

	return ret;
}

