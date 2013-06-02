
#include <arch/paging.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <kernel/common/numaMemoryBank.h>
#include <kernel/common/processTrib/processTrib.h>


#define NUMAMEMBANK_DEFINDEX_NONE	(-1)

numaMemoryBankC::numaMemoryBankC(numaBankId_t id)
:
id(id), rangePtrCache(sizeof(numaMemoryBankC::rangePtrS))
{
	ranges.rsrc = __KNULL;
	defRange.rsrc = __KNULL;
}

numaMemoryBankC::~numaMemoryBankC(void)
{
	rangePtrS	*tmp;

	do
	{
		ranges.lock.writeAcquire();

		tmp = ranges.rsrc;
		if (tmp != __KNULL) {
			ranges.rsrc = ranges.rsrc->next;
		};

		ranges.lock.writeRelease();

		if (tmp == __KNULL) {
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

void numaMemoryBankC::dump(void)
{
	uarch_t		rwFlags, rwFlags2;

	ranges.lock.readAcquire(&rwFlags);
	defRange.lock.readAcquire(&rwFlags2);

	__kprintf(NOTICE NUMAMEMBANK"%d: Dumping. Default range base 0x%P.\n",
		id, defRange.rsrc->baseAddr);

	defRange.lock.readRelease(rwFlags2);

	for (rangePtrS *cur = ranges.rsrc; cur != __KNULL; cur = cur->next)
	{
		__kprintf((utf8Char *)"\tMem range: base 0x%X, size 0x%X.\n",
			cur->range->baseAddr, cur->range->size);

		cur->range->dump();
	};

	ranges.lock.readRelease(rwFlags);
}

error_t numaMemoryBankC::__kspaceAddMemoryRange(
	void *ptrNodeMem, numaMemoryRangeC *__kspace, void *__kspaceInitMem
	)
{
	ranges.rsrc = static_cast<rangePtrS *>( ptrNodeMem );
	ranges.rsrc->range = __kspace;
	defRange.rsrc = __kspace;

	return ranges.rsrc->range->initialize(__kspaceInitMem);
}

error_t numaMemoryBankC::addMemoryRange(paddr_t baseAddr, paddr_t size)
{
	numaMemoryRangeC	*memRange;
	rangePtrS		*tmpNode;
	error_t			err;

	// Allocate a new bmp allocator.
	memRange = new (processTrib.__kgetStream()->memoryStream.memAlloc(
		PAGING_BYTES_TO_PAGES(sizeof(numaMemoryRangeC)),
		MEMALLOC_NO_FAKEMAP))
			numaMemoryRangeC(baseAddr, size);

	if (memRange == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};

	err = memRange->initialize();
	if (err != ERROR_SUCCESS)
	{
		processTrib.__kgetStream()->memoryStream.memFree(memRange);
		return err;
	};

	tmpNode = new (rangePtrCache.allocate()) rangePtrS;
	if (tmpNode == __KNULL)
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
	if (defRange.rsrc == __KNULL) {
		defRange.rsrc = memRange;
	};

	defRange.lock.writeRelease();
	ranges.lock.writeRelease();

	__kprintf(NOTICE NUMAMEMBANK"%d: New mem range: base 0x%X, size 0x%X, "
		"v 0x%X.\n",
		id, baseAddr, size, memRange);

	return ERROR_SUCCESS;
}

error_t numaMemoryBankC::removeMemoryRange(paddr_t baseAddr)
{
	rangePtrS		*cur, *prev=__KNULL;

	ranges.lock.writeAcquire();

	for (cur = ranges.rsrc; cur != __KNULL; )
	{
		if (cur->range->identifyPaddr(baseAddr))
		{
			defRange.lock.writeAcquire();

			if (defRange.rsrc == cur->range) {
				defRange.rsrc = __KNULL;
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

			__kprintf(NOTICE NUMAMEMBANK"%d: Destroying mem range: "
				"base 0x%X, size 0x%X, v 0x%X.\n",
				id, cur->range->baseAddr, cur->range->size,
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
	__kprintf(NOTICE NUMAMEMBANK"%d: Failed to remove range with base 0x%X."
		"\n", id, baseAddr);

	return ERROR_INVALID_ARG_VAL;
}

error_t numaMemoryBankC::contiguousGetFrames(
	uarch_t nFrames, paddr_t *paddr, ubit32)
{
	uarch_t		rwFlags, rwFlags2;
	error_t		ret;

	defRange.lock.readAcquire(&rwFlags);

	if (defRange.rsrc == __KNULL)
	{
		// Check and see if any new ranges have been added recently.
		ranges.lock.readAcquire(&rwFlags2);

		if (ranges.rsrc == __KNULL)
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
	for (rangePtrS *cur = ranges.rsrc; cur != __KNULL; cur = cur->next)
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

status_t numaMemoryBankC::fragmentedGetFrames(
	uarch_t nFrames, paddr_t *paddr, ubit32)
{
	uarch_t		rwFlags, rwFlags2;
	error_t		ret;

	defRange.lock.readAcquire(&rwFlags);

	if (defRange.rsrc == __KNULL)
	{
		// Check and see if any new ranges have been added recently.
		ranges.lock.readAcquire(&rwFlags2);

		if (ranges.rsrc == __KNULL)
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
	for (rangePtrS *cur = ranges.rsrc; cur != __KNULL; cur = cur->next)
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

void numaMemoryBankC::releaseFrames(paddr_t basePaddr, uarch_t nFrames)
{
	uarch_t		rwFlags;

	ranges.lock.readAcquire(&rwFlags);

	for (rangePtrS *cur = ranges.rsrc; cur != __KNULL; cur = cur->next)
	{
		if (cur->range->identifyPaddr(basePaddr))
		{
			cur->range->releaseFrames(basePaddr, nFrames);

			ranges.lock.readRelease(rwFlags);
			return;
		};
	};

	ranges.lock.readRelease(rwFlags);

	__kprintf(WARNING NUMAMEMBANK"%d: releaseFrames(0x%X, %d) Pmem leak.\n",
		id, basePaddr, nFrames);
}

sarch_t numaMemoryBankC::identifyPaddr(paddr_t paddr)
{
	uarch_t		rwFlags;

	ranges.lock.readAcquire(&rwFlags);

	for (rangePtrS *cur = ranges.rsrc; cur != __KNULL; cur = cur->next)
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

sarch_t numaMemoryBankC::identifyPaddrRange(paddr_t basePaddr, paddr_t nBytes)
{
	uarch_t		rwFlags;

	ranges.lock.readAcquire(&rwFlags);

	for (rangePtrS *cur = ranges.rsrc; cur != __KNULL; cur = cur->next)
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

void numaMemoryBankC::mapMemUsed(paddr_t baseAddr, uarch_t nFrames)
{
	uarch_t		rwFlags;

	ranges.lock.readAcquire(&rwFlags);

	for (rangePtrS *cur = ranges.rsrc; cur != __KNULL; cur = cur->next) {
		cur->range->mapMemUsed(baseAddr, nFrames);
	};
	ranges.lock.readRelease(rwFlags);
}

void numaMemoryBankC::mapMemUnused(paddr_t baseAddr, uarch_t nFrames)
{
	uarch_t		rwFlags;

	ranges.lock.readAcquire(&rwFlags);

	for (rangePtrS *cur = ranges.rsrc; cur != __KNULL; cur = cur->next) {
		cur->range->mapMemUnused(baseAddr, nFrames);
	};

	ranges.lock.readRelease(rwFlags);
}

status_t numaMemoryBankC::merge(numaMemoryBankC *nmb)
{
	uarch_t		rwFlags, rwFlags2;
	status_t	ret=0;

	ranges.lock.readAcquire(&rwFlags);
	nmb->ranges.lock.readAcquire(&rwFlags2);

	for (rangePtrS *cur = ranges.rsrc; cur != __KNULL; cur = cur->next)
	{
		for (rangePtrS *cur2 = nmb->ranges.rsrc; cur2 != __KNULL;
			cur2 = cur2->next)
		{
			ret += cur->range->merge(cur2->range);
		};
	};

	nmb->ranges.lock.readRelease(rwFlags2);
	ranges.lock.readRelease(rwFlags);

	return ret;
}

