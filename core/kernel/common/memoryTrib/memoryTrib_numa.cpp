
#include <scaling.h>
#include <chipset/memory.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/numaMemoryBank.h>
#include <kernel/common/memoryTrib/memoryTrib.h>


// The __kspace allocatable memory range's containing memory bank.
static numaMemoryBankC			__kspaceMemoryBank(
	CHIPSET_MEMORY_NUMA___KSPACE_BANKID);

// The pointer node on the __kspace bank that points to the __kspace mem range.
static numaMemoryBankC::rangePtrS	__kspaceRangePtrMem;
// The __kspace mem range, which has a frame cache and a BMP.
static numaMemoryRangeC			__kspaceMemoryRange(
	CHIPSET_MEMORY___KSPACE_BASE,
	CHIPSET_MEMORY___KSPACE_SIZE);

error_t memoryTribC::__kspaceInit(void)
{
	error_t		ret;

	/**	EXPLANATION:
	 * This function literally assembles the boot time physical memory
	 * allocator (__kspace) for the chipset's __kspace region, by simply
	 * putting together pre-allocated and initialized memory management
	 * structures.
	 *
	 * The comments below should suffice. We essentially set up an array
	 * of NUMA banks with memory on them, and place __kspace on it as a fake
	 * bank. Then we add the __kspace memory region to the bank.
	 **/

	defaultAffinity.def.rsrc = CHIPSET_MEMORY_NUMA___KSPACE_BANKID;

	// First give the list class pre-allocated memory to use for its array.
	memoryBanks.__kspaceSetState(
		CHIPSET_MEMORY_NUMA___KSPACE_BANKID,
		static_cast<void *>( initialMemoryBankArray ));

	// Next give it the pre-allocated __kspace memory bank.
	ret = memoryBanks.addItem(
		CHIPSET_MEMORY_NUMA___KSPACE_BANKID, &__kspaceMemoryBank);

	if (ret != ERROR_SUCCESS) {
		return ret;
	};

	nBanks = 1;

	/* Next give it the pre-allocated __kspace memory range object.
	 *
	 * NOTE: numaMemoryBankC already calls initialize() on a
	 * numaMemoryRangeC object when it's adding it to its internal list.
	 *
	 * Do not explicitly call initialize() on the numaMemoryRangeC object
	 * before giving it to the __kspace numaMemoryBankC object.
	 **/
	ret = getBank(CHIPSET_MEMORY_NUMA___KSPACE_BANKID)
		->__kspaceAddMemoryRange(
			&__kspaceRangePtrMem,
			&__kspaceMemoryRange,
			__kspaceInitMem);

	return ret;
}
	
error_t memoryTribC::createBank(numaBankId_t id)
{
	error_t		ret;
	numaMemoryBankC	*nmb;

	// Note the MEMALLOC_NO_FAKEMAP flag: MM code/data should never pgfault.
	nmb = new (
		(memoryTrib.__kmemoryStream
			.*memoryTrib.__kmemoryStream.memAlloc)(
				PAGING_BYTES_TO_PAGES(sizeof(numaMemoryBankC)),
				MEMALLOC_NO_FAKEMAP))
		numaMemoryBankC(id);

	if (nmb == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};

	ret = memoryBanks.addItem(id, nmb);
	if (ret != ERROR_SUCCESS) {
		memoryTrib.__kmemoryStream.memFree(nmb);
	};

	return ret;
}

void memoryTribC::destroyBank(numaBankId_t id)
{
	memoryBanks.removeItem(id);
}

void memoryTribC::releaseFrames(paddr_t paddr, uarch_t nFrames)
{
	numaMemoryBankC	*currBank;
	numaBankId_t	cur;

	/**	EXPLANATION:
	 * Here we have the result of a trade-off. For a reduction in the size
	 * of each entry in allocTableC and better design overall, and smaller
	 * memory footprint, we traded off speed on memory frees.
	 *
	 * The kernel now has to decipher which bank a range of pmem belongs to
	 * before it can be freed.
	 **/
#if __SCALING__ >= SCALING_CC_NUMA
	cur = memoryBanks.prepareForLoop();
	currBank = (numaMemoryBankC *)memoryBanks.getLoopItem(&cur);

	for (; currBank != __KNULL;
		currBank = (numaMemoryBankC *)memoryBanks.getLoopItem(&cur))
	{
		if (currBank->identifyPaddr(paddr))
		{
			currBank->releaseFrames(paddr, nFrames);
			return;
		};
	};
	/* Couldn't find a suitable bank. Probably the memory was hot swapped,
	 * or there's corruption in the memory manager somewhere.
	 **/
	__kprintf(WARNING MEMTRIB"releaseFrames(0x%P, %d): pmem leak.\n",
		paddr, nFrames);
#else
	getBank(defaultAffinity.def.rsrc)
		->releaseFrames(paddr, nFrames);
#endif

}

error_t memoryTribC::contiguousGetFrames(uarch_t nPages, paddr_t *paddr)
{
#if __SCALING__ >= SCALING_CC_NUMA
	numaBankId_t		def;
	numaBankId_t		cur;
	uarch_t			rwFlags;
#endif
	error_t			ret;
	numaMemoryBankC		*currBank;

#if __SCALING__ >= SCALING_CC_NUMA
	// Get the default bank's Id.
	defaultAffinity.def.lock.readAcquire(&rwFlags);
	def = defaultAffinity.def.rsrc;
	defaultAffinity.def.lock.readRelease(rwFlags);

	currBank = getBank(def);
#else
	// Allocate from default bank, which is the sharedBank for non-NUMA.
	currBank = getBank(defaultAffinity.def.rsrc);
#endif

	// FIXME: Decide what to do here for an non-NUMA build.
	if (currBank != __KNULL)
	{
		ret = currBank->contiguousGetFrames(nPages, paddr);
		if (ret == ERROR_SUCCESS) {
			return ret;
		};
	};

#if __SCALING__ >= SCALING_CC_NUMA
	/* If we're still here then there was no memory on the default bank. We
	 * have to now determine which bank would have memory, and set that to
	 * be the new default bank for raw contiguous allocations.
	 **/
	def = cur = memoryBanks.prepareForLoop();
	currBank = (numaMemoryBankC *)memoryBanks.getLoopItem(&def);

	for (; currBank != __KNULL;
		currBank = (numaMemoryBankC *)memoryBanks.getLoopItem(&def))
	{
		// Attempt to allocate from the current stream.
		ret = currBank->contiguousGetFrames(nPages, paddr);
		if (ret == ERROR_SUCCESS)
		{
			// Set the current stream to be the new default.
			defaultAffinity.def.lock.writeAcquire();
			defaultAffinity.def.rsrc = cur;
			defaultAffinity.def.lock.writeRelease();
			// Return. We got memory off the current bank.
			return ret;
		};
		cur = def;
	};
#endif
	// If we reached here, then we've failed completely.
	return ERROR_MEMORY_NOMEM_PHYSICAL;
}

error_t memoryTribC::fragmentedGetFrames(uarch_t nPages, paddr_t *paddr)
{
#if __SCALING__ >= SCALING_CC_NUMA
	numaBankId_t		def;
	numaBankId_t		cur;
	uarch_t			rwFlags;
#endif
	error_t			ret;
	numaMemoryBankC		*currBank;

#if __SCALING__ >= SCALING_CC_NUMA
	// Get the default bank's Id.
	defaultAffinity.def.lock.readAcquire(&rwFlags);
	def = defaultAffinity.def.rsrc;
	defaultAffinity.def.lock.readRelease(rwFlags);

	currBank = getBank(def);
#else
	// Allocate from default bank, which is the sharedBank for non-NUMA.
	currBank = getBank(defaultAffinity.def.rsrc);
#endif

	// FIXME: Decide what to do here for an non-NUMA build.
	if (currBank != __KNULL)
	{
		ret = currBank->fragmentedGetFrames(nPages, paddr);
		if (ret > 0) {
			return ret;
		};
	};

#if __SCALING__ >= SCALING_CC_NUMA
	/* If we're still here then there was no memory on the default bank. We
	 * have to now determine which bank would have memory, and set that to
	 * be the new default bank for raw contiguous allocations.
	 **/
	def = cur = memoryBanks.prepareForLoop();
	currBank = (numaMemoryBankC *)memoryBanks.getLoopItem(&def);

	for (; currBank != __KNULL;
		currBank = (numaMemoryBankC *)memoryBanks.getLoopItem(&def))
	{
		// Attempt to allocate from the current stream.
		ret = currBank->fragmentedGetFrames(nPages, paddr);

		if (ret > 0)
		{
			// Set the current stream to be the new default.
			defaultAffinity.def.lock.writeAcquire();
			defaultAffinity.def.rsrc = cur;
			defaultAffinity.def.lock.writeRelease();
			// Return. We got memory off the current bank.
			return ret;
		};
		cur = def;
	};
#endif
	// If we reached here, then we've failed completely.
	return ERROR_MEMORY_NOMEM_PHYSICAL;
}

// Preprocess out this whole function on a non-NUMA build.
#if __SCALING__ >= SCALING_CC_NUMA
error_t memoryTribC::configuredGetFrames(
	localAffinityS *aff, uarch_t nPages, paddr_t *paddr
	)
{
	numaBankId_t		def, cur;
	numaMemoryBankC		*currBank;
	error_t			ret;
	uarch_t			rwFlags;

	// Get the thread's default config.
	aff->def.lock.readAcquire(&rwFlags);
	def = aff->def.rsrc;
	aff->def.lock.readRelease(rwFlags);

	currBank = getBank(def);
	if (currBank != __KNULL)
	{
		ret = currBank->fragmentedGetFrames(nPages, paddr);

		if (ret > 0) {
			return ret;
		};
	};

	// Allocation from the default bank failed. Find another default bank.
	def = cur = memoryBanks.prepareForLoop();
	currBank = (numaMemoryBankC *)memoryBanks.getLoopItem(&def);

	for (; currBank != __KNULL;
		currBank = (numaMemoryBankC *)memoryBanks.getLoopItem(&def))
	{
		// If this bank is part of the thread's NUMA policy:
		if (aff->memBanks.testSingle(cur))
		{
			ret = currBank->fragmentedGetFrames(nPages, paddr);
			if (ret > 0)
			{
				aff->def.lock.writeAcquire();
				aff->def.rsrc = cur;
				aff->def.lock.writeRelease();
				return ret;
			};
		};
		cur = def;
	};

	/* If we reach here then none of the thread's configured banks have any
	 * physical mem left on them.
	 **/
	return 0;
}
#endif

void memoryTribC::mapRangeUsed(paddr_t baseAddr, uarch_t nPages)
{
#if __SCALING__ >= SCALING_CC_NUMA
	numaBankId_t	cur;
	numaMemoryBankC	*currBank;
#endif


#if __SCALING__ >= SCALING_CC_NUMA
	cur = memoryBanks.prepareForLoop();
	currBank = (numaMemoryBankC *)memoryBanks.getLoopItem(&cur);
	
	for (; currBank != __KNULL;
		currBank = (numaMemoryBankC *)memoryBanks.getLoopItem(&cur))
	{
		/* We can most likely afford this small speed bump since ranges
		 * of physical RAM are not often mapped or unmapped as used at
		 * runtime. This generally only happens when the kernel is made
		 * aware of an MMIO range.
		 **/
		currBank->mapMemUsed(baseAddr, nPages);
	};
#else
	getBank(defaultAffinity.def.rsrc)->mapMemUsed(baseAddr, nPages);
#endif
}

void memoryTribC::mapRangeUnused(paddr_t baseAddr, uarch_t nPages)
{
#if __SCALING__ >= SCALING_CC_NUMA
	numaBankId_t	cur;
	numaMemoryBankC	*currBank;
#endif


#if __SCALING__ >= SCALING_CC_NUMA
	cur = memoryBanks.prepareForLoop();
	currBank = (numaMemoryBankC *)memoryBanks.getLoopItem(&cur);
	
	for (; currBank != __KNULL;
		currBank = (numaMemoryBankC *)memoryBanks.getLoopItem(&cur))
	{
		/* We can most likely afford this small speed bump since ranges
		 * of physical RAM are not often mapped or unmapped as used at
		 * runtime. This generally only happens when the kernel is made
		 * aware of an MMIO range.
		 **/
		currBank->mapMemUnused(baseAddr, nPages);
	};
#else
	getBank(defaultAffinity.def.rsrc)->mapMemUnused(baseAddr, nPages);
#endif
}

