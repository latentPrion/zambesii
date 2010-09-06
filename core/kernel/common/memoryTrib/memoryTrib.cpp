
#include <arch/walkerPageRanger.h>
#include <lang/lang.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <kernel/common/processId.h>
#include <kernel/common/panic.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/numaTrib/numaTrib.h>


memoryTribC::memoryTribC(
	pagingLevel0S *level0Accessor, paddr_t level0Paddr
	)
:
__kmemoryStream(__KPROCESSID, level0Accessor, level0Paddr)
{
	for (uarch_t i=0; i<CHIPSET_MEMORY_NREGIONS; i++)
	{
		memRegions[i].info = __KNULL;
		memRegions[i].memBmp = __KNULL;
	};
}

// Initializes the kernel's Memory Stream.
error_t memoryTribC::initialize(
	void *swampStart, uarch_t swampSize, vSwampC::holeMapS *holeMap
	)
{
	return __kmemoryStream.initialize(swampStart, swampSize, holeMap);
}

error_t memoryTribC::initialize2(void)
{
	chipsetRegionMapEntryS		*currEntry;
	chipsetRegionReservedS		*currReserved;
	paddr_t				currBase, currSize;
	void				*bmpMem;

	/**	EXPLANATION:
	 * On a call to memoryTribC::initialize2() the kernel proceed to spawn
	 * the bitmaps for all memory regions.
	 *
	 * These memory region bitmaps depend on Kernel Space RAM to initialize
	 * since this is the only usable RAM that the kernel is aware of;
	 * NUMA Tributary has not yet been initialize()d, so we at this point,
	 * have no idea of how much RAM is on the board, how many NUMA banks,
	 * if that applies, and what parts of RAM are usable. We only have
	 * Kernel Space RAM which is a hardcoded range of RAM of size N bytes
	 * which the person who ported us to this chipset told us about, that
	 * we could assume is guaranteed safe RAM.
	 **/
	// Return success if the chipset has no regions whatsoever.
	if (chipsetRegionMap == __KNULL) { return ERROR_SUCCESS; };

	/* We want to run through each chipsetRegionMapEntryS entry and for
	 * each one, generate a BMP at runtime. The bitmaps for the chipset
	 * memory regions are allocated from Kernel Space.
	 **/
	for (uarch_t i=0; i<chipsetRegionMap->nEntries; i++)
	{
		currEntry = &chipsetRegionMap->entries[i];
		currReserved = currEntry->reservedMap;

		currBase = currEntry->baseAddr;
		currSize = currEntry->size;

		bmpMem = rawMemAlloc(1, 0);
		if (bmpMem == __KNULL) {
			return ERROR_MEMORY_NOMEM;
		};

		memRegions[i].memBmp = new (bmpMem) memBmpC(
			currBase, currSize);

		// If there are any hardcoded reserved ranges, map them in.
		if (currReserved != __KNULL)
		{
			for (uarch_t j=0; i<currEntry->nReservedEntries; i++)
			{
				memRegions[i].memBmp->mapMemUsed(
					currReserved[j].baseAddr,
					PAGING_BYTES_TO_PAGES(
						currReserved[j].size));
			};
		};

		memRegions[i].info = currEntry;
	};
	// Now memory region allocation is half-initialized.
	return ERROR_SUCCESS;
}

// TODO: This function can be greatly optimized. KAGS, you are needed.
void *memoryTribC::rawMemAlloc(uarch_t nPages, uarch_t)
{
	void		*ret;
	paddr_t		paddr;
	uarch_t		totalFrames;
	status_t	nFetched, nMapped;

	ret = (__kmemoryStream.vaddrSpaceStream
		.*__kmemoryStream.vaddrSpaceStream.getPages)(nPages);

	if (ret == __KNULL) {
		return __KNULL;
	};

	/* memoryTribC::rawMemAlloc() has no allocTable. Therefore it is
	 * impossible to do lazy allocation using fakemapped pages. This is
	 * due to the fact that lazy allocation requires a #PF to occur on a
	 * page which has not yet been backed with pmem.
	 *
	 * The kernel will then read the alloc table and see how many pages were
	 * supposed to be given frames, and it will then demand allocate more
	 * frames. But if there is no alloc table, then the #PF will occur, and
	 * then we end up knowing nothing about the allocation.
	 **/
	for (totalFrames = 0; totalFrames < nPages; )
	{
		nFetched = numaTrib.fragmentedGetFrames(
			nPages - totalFrames, &paddr);

		if (nFetched > 0)
		{
			nMapped = walkerPageRanger::mapInc(
				&__kmemoryStream.vaddrSpaceStream.vaddrSpace,
				(void *)((uarch_t)ret
					+ (totalFrames * PAGING_BASE_SIZE)),
				paddr, nFetched,
				PAGEATTRIB_PRESENT | PAGEATTRIB_WRITE
				| PAGEATTRIB_SUPERVISOR);

			if (nMapped < nFetched)
			{
				__kprintf(
					FATAL"MemoryTrib.rawMemAlloc(%d): "
					"walkerPageRanger::mapInc() returned "
					"%d frames mapped.", nPages, nMapped);

				rawMemFree(
					ret,
					totalFrames + static_cast<uarch_t>(
						nMapped ));

				goto returnFailure;
			};

			totalFrames += static_cast<uarch_t>( nFetched );
		};
	};

	return ret;

returnFailure:
	__kmemoryStream.vaddrSpaceStream.releasePages(
		(void *)((uarch_t)ret + ((totalFrames + (uarch_t)nMapped)
			* PAGING_BASE_SIZE)),
		nPages - (totalFrames + nMapped));

	return __KNULL;
}

void memoryTribC::rawMemFree(void *vaddr, uarch_t nPages)
{
	/* rawMemFree() will look up every paddr for every page since there is
	 * no alloc table for rawMemFree().
	 **/
	uarch_t		_nPages, tracker;
	uarch_t		flags;
	paddr_t		paddr;
	status_t	status;

	tracker = reinterpret_cast<uarch_t>( vaddr );

	for (_nPages = nPages;
		_nPages > 0;
		tracker += PAGING_BASE_SIZE, _nPages--)
	{
		status = walkerPageRanger::unmap(
			&__kmemoryStream.vaddrSpaceStream.vaddrSpace,
			reinterpret_cast<void *>( tracker ),
			&paddr, 1, &flags);

		//Only free the paddr if there was a valid mapping in the vaddr.
		if (status == WPRANGER_STATUS_BACKED) {
			numaTrib.releaseFrames(paddr, 1);
		};
	};

	// Now return the virtual addresses to the swamp.
	__kmemoryStream.vaddrSpaceStream.releasePages(vaddr, nPages);
}

