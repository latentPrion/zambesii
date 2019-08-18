
#include <debug.h>
#include <arch/walkerPageRanger.h>
#include <chipset/memory.h>
#include <chipset/zkcm/memoryMap.h>
#include <chipset/zkcm/zkcmCore.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <kernel/common/processId.h>
#include <kernel/common/panic.h>
#include <kernel/common/numaMemoryBank.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/floodplainn/dma.h>


static ubit8				memoryTribAvailableBanksBmpMem[64];
static HardwareIdList::sArrayNode	memoryTribMemoryBanksListMem[
	CHIPSET_NUMA___KSPACE_BANKID + 1];

MemoryTrib::MemoryTrib(void)
:
nBanks(0)
{
	for (uarch_t i=0; i<CHIPSET_MEMORY_NREGIONS; i++)
	{
		memRegions[i].info = NULL;
		memRegions[i].memBmp = NULL;
	};

#if __SCALING__ < SCALING_CC_NUMA
	defaultMemoryBank.rsrc = NUMABANKID_INVALID;
#endif
}

error_t MemoryTrib::initialize(void)
{
	availableBanks.initialize(
		0,
		Bitmap::sPreallocatedMemory(
			memoryTribAvailableBanksBmpMem,
			sizeof(memoryTribAvailableBanksBmpMem)));

	memoryBanks.initialize(
		memoryTribMemoryBanksListMem,
		sizeof(memoryTribMemoryBanksListMem));

	return ERROR_SUCCESS;
}

void MemoryTrib::dump(void)
{
	HardwareIdList::Iterator	it;
	NumaMemoryBank			*nmb;

	printf(NOTICE MEMTRIB"Dumping: %d banks."
#if __SCALING__ < SCALING_CC_NUMA
		" default membank: %d.\n",
		nBanks, defaultMemoryBank.rsrc);
#else
		"\n", nBanks);
#endif

	for (it = memoryBanks.begin(); it != memoryBanks.end(); ++it)
	{
		nmb = (NumaMemoryBank *)*it;
		nmb->dump();
	};
}

error_t MemoryTrib::memRegionInit(void)
{
	sChipsetRegionMapEntry		*currEntry;
	sReservedChipsetRegion		*currReserved;
	sZkcmMemoryMapS			*memMap;
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
	if (chipsetRegionMap == NULL) { return ERROR_SUCCESS; };

	/* We want to run through each sChipsetRegionMapEntry entry and for
	 * each one, generate a BMP at runtime. The bitmaps for the chipset
	 * memory regions are allocated from Kernel Space.
	 **/
	for (uarch_t i=0; i<chipsetRegionMap->nEntries; i++)
	{
		currEntry = &chipsetRegionMap->entries[i];
		currReserved = currEntry->reservedMap;

		currBase = currEntry->baseAddr;
		currSize = currEntry->size;

		memRegions[i].memBmp = new (rawMemAlloc(1, 0)) MemoryBmp(
			currBase, currSize);

		if (memRegions[i].memBmp == NULL) {
			return ERROR_MEMORY_NOMEM;
		};

		ret = memRegions[i].memBmp->initialize();
		if (ret != ERROR_SUCCESS) { return ret; };

		// If there are any hardcoded reserved ranges, mark them used.
		if (currReserved != NULL)
		{
			for (uarch_t j=0; i<currEntry->nReservedEntries; i++)
			{
				memRegions[i].memBmp->mapMemUsed(
					currReserved[j].baseAddr,
					PAGING_BYTES_TO_PAGES(
						currReserved[j].size).getLow());
			};
		};

		memRegions[i].info = currEntry;
	};

	// Next step is to overlay the memory regions with chipset memory map.
	memMap = zkcmCore.memoryDetection.getMemoryMap();
	if (memMap == NULL) { return ERROR_SUCCESS; };

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
						memMap->entries[j].size)
						.getLow());
			};
		};
	};

	return ERROR_SUCCESS;
}

sbit8 MemoryTrib::releaseFrames(fplainn::dma::ScatterGatherList *list)
{
	sbit8								ret = 0;
	fplainn::dma::ScatterGatherList::SGListElementArray::Iterator	it;

	for (it = list->elements.begin(); it != list->elements.end(); ++it)
	{
		sbit8					wasFreed=0;
		fplainn::dma::__kscgth_element_type_t	tmp = *it;
		paddr_t					p;

		assign_scgth_block_busaddr_to_paddr(p, tmp.block_busaddr);
		wasFreed = releaseFrames(
			p, PAGING_BYTES_TO_PAGES(tmp.block_length));

		if (wasFreed) { ret = 1; };
	}

	return ret;
}

// TODO: This function can be greatly optimized. KAGS, you are needed.
void *MemoryTrib::rawMemAlloc(uarch_t nPages, uarch_t flags)
{
	void		*ret;
	paddr_t		paddr;
	uarch_t		totalFrames;
	status_t	nFetched, nMapped;
	sarch_t		fLocalFlush;

	ret = processTrib.__kgetStream()->getVaddrSpaceStream()->getPages(
		nPages);

	if (ret == NULL) {
		return NULL;
	};

	/* MemoryTrib::rawMemAlloc() has no allocTable. Therefore it is
	 * impossible to do lazy allocation using fakemapped pages. This is
	 * due to the fact that lazy allocation requires a #PF to occur on a
	 * page which has not yet been backed with pmem.
	 *
	 * The kernel will then read the alloc table and see how many pages were
	 * supposed to be given frames, and it will then demand allocate more
	 * frames. But if there is no alloc table, then the #PF will occur, and
	 * then we end up knowing nothing about the allocation.
	 **/
	fLocalFlush = FLAG_TEST(flags, MEMALLOC_LOCAL_FLUSH_ONLY) ? 1 : 0;

	for (totalFrames = 0; totalFrames < nPages; )
	{
		nFetched = fragmentedGetFrames(
			nPages - totalFrames, &paddr);

		if (nFetched > 0)
		{
			nMapped = walkerPageRanger::mapInc(
				&processTrib.__kgetStream()
					->getVaddrSpaceStream()->vaddrSpace,
				(void *)((uintptr_t)ret
					+ (totalFrames * PAGING_BASE_SIZE)),
				paddr, nFetched,
				PAGEATTRIB_PRESENT | PAGEATTRIB_WRITE
				| PAGEATTRIB_SUPERVISOR
				| ((fLocalFlush)
					? PAGEATTRIB_LOCAL_FLUSH_ONLY : 0));

			if (nMapped < nFetched)
			{
				printf(
					FATAL"MemoryTrib.rawMemAlloc(%d): "
					"walkerPageRanger::mapInc() returned "
					"%d frames mapped.", nPages, nMapped);

				rawMemFree(
					ret,
					totalFrames + static_cast<uarch_t>(
						nMapped ),
					flags);

				goto returnFailure;
			};

			totalFrames += static_cast<uarch_t>( nFetched );
		};
	};

	return ret;

returnFailure:
	processTrib.__kgetStream()->getVaddrSpaceStream()->releasePages(
		(void *)((uarch_t)ret + ((totalFrames + (uarch_t)nMapped)
			* PAGING_BASE_SIZE)),
		nPages - (totalFrames + nMapped));

	return NULL;
}

void MemoryTrib::rawMemFree(void *vaddr, uarch_t nPages, uarch_t flags)
{
	/* rawMemFree() will look up every paddr for every page since there is
	 * no alloc table for rawMemFree().
	 **/
	uarch_t		_nPages, tracker;
	paddr_t		paddr;
	status_t	status;

	tracker = reinterpret_cast<uarch_t>( vaddr );

	for (_nPages = nPages;
		_nPages > 0;
		tracker += PAGING_BASE_SIZE, _nPages--)
	{
		uarch_t		__kflags=0;

		if (FLAG_TEST(flags, MEMALLOC_LOCAL_FLUSH_ONLY)) {
			FLAG_SET(__kflags, PAGEATTRIB_LOCAL_FLUSH_ONLY);
		};

		status = walkerPageRanger::unmap(
			&processTrib.__kgetStream()->getVaddrSpaceStream()
				->vaddrSpace,
			reinterpret_cast<void *>( tracker ),
			&paddr, 1, &__kflags);

		//Only free the paddr if there was a valid mapping in the vaddr.
		if (status == WPRANGER_STATUS_BACKED) {
			releaseFrames(paddr, 1);
		};
	};

	// Now return the virtual addresses to the swamp.
	processTrib.__kgetStream()->getVaddrSpaceStream()->releasePages(
		vaddr, nPages);
}

