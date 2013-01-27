
#include <arch/walkerPageRanger.h>
#include <chipset/memory.h>
#include <chipset/zkcm/memoryMap.h>
#include <chipset/zkcm/zkcmCore.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <kernel/common/processId.h>
#include <kernel/common/panic.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/processTrib/processTrib.h>


static ubit8				memoryTribAvailableBanksBmpMem[64];
static hardwareIdListC::arrayNodeS	memoryTribMemoryBanksListMem[
	CHIPSET_MEMORY_NUMA___KSPACE_BANKID + 1];

memoryTribC::memoryTribC(void)
:
	nBanks(0)
{
	for (uarch_t i=0; i<CHIPSET_MEMORY_NREGIONS; i++)
	{
		memRegions[i].info = __KNULL;
		memRegions[i].memBmp = __KNULL;
	};

#if __SCALING__ < SCALING_CC_NUMA
	defaultMemoryBank.rsrc = NUMABANKID_INVALID;
#endif
}

error_t memoryTribC::initialize(void)
{
	availableBanks.initialize(
		0, memoryTribAvailableBanksBmpMem,
		sizeof(memoryTribAvailableBanksBmpMem));

	memoryBanks.initialize(
		memoryTribMemoryBanksListMem,
		sizeof(memoryTribMemoryBanksListMem));

	return ERROR_SUCCESS;
}

error_t memoryTribC::memRegionInit(void)
{
	chipsetRegionMapEntryS		*currEntry;
	chipsetRegionReservedS		*currReserved;
	zkcmMemMapS			*memMap;
	paddr_t				currBase, currSize;
	error_t				ret;

	/**	EXPLANATION:
	 * The kernel allocates PMM structures for the special memory regions on
	 * a chipset only after the rest of the physical and virtual memory
	 * management subsystems have been initialized. This is because the
	 * physical memory management associated with a chipset's special
	 * memory regions is generally unnecessary at boot.
	 *
	 * To set up special regions, the kernel must parse the region map, and
	 * spawn a PMM structure for each.
	 *
	 * Following this, the kernel will overlay each of the memory regions
	 * with the memory map which the chipset is supposed to have provided.
	 *
	 * This memory map overlay will ensure that no reserved RAM areas which
	 * are contained in a special region will be left marked as allocatable.
	 **/
	// If there are no memory regions, just exit.
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

		memRegions[i].memBmp = new (rawMemAlloc(1, 0)) memBmpC(
			currBase, currSize);

		if (memRegions[i].memBmp == __KNULL) {
			return ERROR_MEMORY_NOMEM;
		};

		ret = memRegions[i].memBmp->initialize();
		if (ret != ERROR_SUCCESS) { return ret; };

		// If there are any hardcoded reserved ranges, mark them used.
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

	// Next step is to overlay the memory regions with chipset memory map.
	memMap = zkcmCore.memoryDetection.getMemoryMap();
	if (memMap == __KNULL) { return ERROR_SUCCESS; };

	for (ubit32 i=0; i<chipsetRegionMap->nEntries; i++)
	{
		for (ubit32 j=0; j<memMap->nEntries; j++)
		{
			// Mark any memory that's not usable as used.
			if (memMap->entries[j].memType
				!= ZKCM_MMAP_TYPE_USABLE)
			{
				memRegions[i].memBmp->mapMemUsed(
					memMap->entries[j].baseAddr,
					PAGING_BYTES_TO_PAGES(
						memMap->entries[j].size));
			};
		};
	};

	return ERROR_SUCCESS;
}

// TODO: This function can be greatly optimized. KAGS, you are needed.
void *memoryTribC::rawMemAlloc(uarch_t nPages, uarch_t)
{
	void		*ret;
	paddr_t		paddr;
	uarch_t		totalFrames;
	status_t	nFetched, nMapped;

	ret = processTrib.__kprocess.memoryStream.vaddrSpaceStream.getPages(
		nPages);

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
		nFetched = fragmentedGetFrames(
			nPages - totalFrames, &paddr);

		if (nFetched > 0)
		{
			nMapped = walkerPageRanger::mapInc(
				&processTrib.__kprocess.memoryStream
					.vaddrSpaceStream.vaddrSpace,
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
	processTrib.__kprocess.memoryStream.vaddrSpaceStream.releasePages(
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
			&processTrib.__kprocess.memoryStream
				.vaddrSpaceStream.vaddrSpace,
			reinterpret_cast<void *>( tracker ),
			&paddr, 1, &flags);

		//Only free the paddr if there was a valid mapping in the vaddr.
		if (status == WPRANGER_STATUS_BACKED) {
			releaseFrames(paddr, 1);
		};
	};

	// Now return the virtual addresses to the swamp.
	processTrib.__kprocess.memoryStream.vaddrSpaceStream.releasePages(
		vaddr, nPages);
}

