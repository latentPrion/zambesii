
#include <arch/walkerPageRanger.h>
#include <lang/mm.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/processId.h>
#include <kernel/common/panic.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/numaTrib/numaTrib.h>

memoryTribC::memoryTribC(
	pagingLevel0S *level0Accessor, paddr_t level0Paddr
	)
:
__kmemoryStream(__KPROCESSID, level0Accessor, level0Paddr),
{
	memset(
		memRegions, 0,
		sizeof(memoryRegionC *) * CHIPSET_MEMORY_NREGIONS);
}

// Initializes the kernel's Memory Stream.
error_t memoryTribC::initialize(
	void *swampStart, uarch_t swampSize, vSwampC::holeMapS *holeMap
	)
{
	chipsetRegionMapEntryS		*currEntry;
	chipsetRegionReservedS		*currReserved;
	paddr_t				currBase, currSize;
	void				*bmpMem;
	error_t				ret;

	/**	EXPLANATION:
	 * On a call to memoryTribC::initialize() the kernel will first call
	 * initialize() on its Memory Stream. If that returns successfully, it
	 * will then proceed to spawn the bitmaps for all memory regions.
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
	ret = __kmemoryStream.initialize(swampStart, swampSize, holeMap);
	if (ret != ERROR_SUCCESS) { return ret; };

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

		bmpMem = rawMemAlloc(1);
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
				memRegions[i].memBmp->mapRangeUsed(
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

/**	NOTE:
 * It may be beneficial to change the way we try to allocate pmem: Maybe
 * instead of just trying to find pmem only once, we should try to do it
 * multiple times, until we get a NULL delivery, and iteratively map all the
 * frames we got so far. Thus we get as many frames as we can before returning.
 *
 * Then fake map any vmem that didn't get pmem.
 **/
void *memoryTribC::rawMemAlloc(uarch_t nPages)
{
	void		*ret;
	error_t		nFound;
	status_t	nMapped;
	paddr_t		paddr;

	/* This method will explicitly allocate memory from the kernel's
	 * vaddrspace, and it is not very concerned about where the memory
	 * comes from. It just gets memory from the NUMA Tributary. Allocation
	 * is done via numaMemoryBankC::fragmentedGetFrames(), so the kernel
	 * receives RAM from pretty much anywhere, on any bank.
	 **/
	// First see if the kernel address space has enough vmem for the alloc.
	ret = (__kmemoryStream.vaddrSpaceStream
		.*__kmemoryStream.vaddrSpaceStream.getPages)(nPages);

	if (ret == __KNULL)
	{
		// Leave the handling of this problem to the caller.
		return __KNULL;
	};

	// We must get at least one frame of backing mem.
	do
	{
		nFound = numaTrib.fragmentedGetFrames(nPages, &paddr);
	} while (nFound == 0);

	// We have N pages that are backed, and N that aren't.
	nMapped = walkerPageRanger::__kdataMap(
		&__kmemoryStream.vaddrSpaceStream.vaddrSpace,
		ret, paddr, nFound);

	if (nMapped < nFound) {
		panic(mmStr[1]);
	};

	// Fakemap the pages that aren't physically backed with RAM.
	nMapped = walkerPageRanger::__kfakeMap(
		&__kmemoryStream.vaddrSpaceStream.vaddrSpace,
		reinterpret_cast<void *>(
			reinterpret_cast<uarch_t>( ret )
				+ (nFound * PAGING_BASE_SIZE) ),
		nPages - nFound,
		PAGEATTRIB_WRITE);

	if (nMapped < static_cast<sarch_t>( nPages ) - nFound) {
		panic(mmStr[2]);
	};

	return ret;
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
			&paddr, 1, &flags, 0);

		//Only free the paddr if there was a valid mapping in the vaddr.
		if (status == WPRANGER_STATUS_BACKED) {
			numaTrib.releaseFrames(paddr, 1);
		};
	};

	// Now return the virtual addresses to the swamp.
	__kmemoryStream.vaddrSpaceStream.releasePages(vaddr, nPages);
}

