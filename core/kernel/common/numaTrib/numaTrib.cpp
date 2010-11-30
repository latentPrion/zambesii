
#include <debug.h>
#include <scaling.h>
#include <arch/paging.h>
#include <chipset/memory.h>
#include <chipset/numaMap.h>
#include <chipset/memoryMap.h>
#include <chipset/memoryConfig.h>
#include <chipset/regionMap.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/numaTrib/numaTrib.h>
#include <kernel/common/firmwareTrib/firmwareTrib.h>

/**	EXPLANATION:
 * To initialize the NUMA Tributary, the steps are:
 *	1. Pre-allocate an instance of NUMA StreamC. This stream must be
 *	   given the ID configured for CHIPSET_MEMORY_NUMA___KSPACE_BANKID.
 *	2. Pre-allocate an array of bytes with enough space to hold an array
 *	   of elements of size:
 *	   	sizeof(hardwareIdListC::arrayNodeS) * ({__kspace bankid} + 1).
 *
 *	3. Initialize a numaMemoryRangeC instance, and set it such that its
 *	   base physical address and size are those configured for
 *	   CHIPSET_MEMORY___KSPACE_BASE and CHIPSET_MEMORY___KSPACE_SIZE.
 *	4. Pre-allocate enough memory for the numaMemoryBankC object to use as
 *	   the single array index to point to the __kspace memory range.
 *	5. In numaTribC::initialize():
 *		a. Call numaStreams.__kspaceSetState() with the address of the
 *		   memory pre-allocated to hold the array with enough space to
 *		   hold the index for __kspace (from (2) above).
 *		b. Call numaStreams.addItem() with the configured bank ID of
 *		   __kspace, and the address of the __kspace NUMA stream.
 *		c. Now call getStream() with the __kspace bank Id, and call
 *		   __kspaceAddMemoryRange() on the handle with the address
 *		   of the __kspace memoryRange object, its array pointer index,
 *		   and the __kspaceInitMem which the chipset has reserved for
 *		   it.
 *
 * CAVEATS:
 * ^ __kspace must have its own bank which is guaranteed NOT to clash with any
 *   other bank ID during numaTribC::initialize2().
 **/


// Initialize the __kspace NUMA Stream to its configured Stream ID.
static numaStreamC	__kspaceNumaStream(CHIPSET_MEMORY_NUMA___KSPACE_BANKID);
// Space for the numaMemoryBank's array.
static numaMemoryBankC::rangePtrS	__kspaceRangePtrMem;
static numaMemoryRangeC			__kspaceMemoryRange(
	CHIPSET_MEMORY___KSPACE_BASE,
	CHIPSET_MEMORY___KSPACE_SIZE);


numaTribC::numaTribC(void)
{
	defaultAffinity.def.rsrc = CHIPSET_MEMORY_NUMA___KSPACE_BANKID;
	nStreams = 0;
#ifdef CHIPSET_MEMORY_NUMA_GENERATE_SHBANK
	sharedBank = NUMATRIB_SHBANK_INVALID;
#endif
}

error_t numaTribC::initialize(void)
{
	error_t		ret;
	numaStreams.__kspaceSetState(
		CHIPSET_MEMORY_NUMA___KSPACE_BANKID,
		static_cast<void *>( __kspaceStreamPtr ));

	ret = numaStreams.addItem(
		CHIPSET_MEMORY_NUMA___KSPACE_BANKID, &__kspaceNumaStream);

	if (ret != ERROR_SUCCESS) {
		return ret;
	};

	nStreams = 1;
	ret = getStream(CHIPSET_MEMORY_NUMA___KSPACE_BANKID)
		->memoryBank.__kspaceAddMemoryRange(
			&__kspaceRangePtrMem,
			&__kspaceMemoryRange,
			__kspaceInitMem);

	return ret;
}

void numaTribC::dump(void)
{
	numaBankId_t	cur;
	numaStreamC	*curStream;

	__kprintf(NOTICE NUMATRIB"Dumping. nStreams %d.\n", nStreams);

	cur = numaStreams.prepareForLoop();
	curStream = (numaStreamC *)numaStreams.getLoopItem(&cur);

	for (; curStream != __KNULL;
		curStream = (numaStreamC *)numaStreams.getLoopItem(&cur))
	{
		curStream->memoryBank.dump();
	}
}

error_t numaTribC::spawnStream(numaBankId_t id)
{
	error_t		ret;
	numaStreamC	*ns;

	ns = new (
		(memoryTrib.__kmemoryStream
			.*memoryTrib.__kmemoryStream.memAlloc)(
				PAGING_BYTES_TO_PAGES(sizeof(numaStreamC)),
				MEMALLOC_NO_FAKEMAP))
		numaStreamC(id);

	if (ns == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};

	ret = numaStreams.addItem(id, ns);
	if (ret != ERROR_SUCCESS) {
		memoryTrib.__kmemoryStream.memFree(ns);
	};

	return ret;
}

numaTribC::~numaTribC(void)
{
	nStreams = 0;
}

void numaTribC::releaseFrames(paddr_t paddr, uarch_t nFrames)
{
	numaStreamC	*currStream;
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
	cur = numaStreams.prepareForLoop();
	currStream = (numaStreamC *)numaStreams.getLoopItem(&cur);

	for (; currStream != __KNULL;
		currStream = (numaStreamC *)numaStreams.getLoopItem(&cur))
	{
		if (currStream->memoryBank.identifyPaddr(paddr))
		{
			currStream->memoryBank.releaseFrames(paddr, nFrames);
			return;
		};
	};
	/* Couldn't find a suitable bank. Probably the memory was hot swapped,
	 * or there's corruption in the memory manager somewhere.
	 **/
	__kprintf(WARNING NUMATRIB"releaseFrames(0x%X, %d): pmem leak.\n",
		paddr, nFrames);
#else
	getStream(defaultAffinity.def.rsrc)
		->memoryBank.releaseFrames(paddr, nFrames);
#endif

}

error_t numaTribC::contiguousGetFrames(uarch_t nPages, paddr_t *paddr)
{
#if __SCALING__ >= SCALING_CC_NUMA
	numaBankId_t		def;
	numaBankId_t		cur;
	uarch_t			rwFlags;
#endif
	error_t			ret;
	numaStreamC		*currStream;

#if __SCALING__ >= SCALING_CC_NUMA
	// Get the default bank's Id.
	defaultAffinity.def.lock.readAcquire(&rwFlags);
	def = defaultAffinity.def.rsrc;
	defaultAffinity.def.lock.readRelease(rwFlags);

	currStream = getStream(def);	
#else
	// Allocate from default bank, which is the sharedBank for non-NUMA.
	currStream = getStream(defaultAffinity.def.rsrc);
#endif

	// FIXME: Decide what to do here for an non-NUMA build.
	if (currStream != __KNULL)
	{
		ret = currStream->memoryBank.contiguousGetFrames(nPages, paddr);
		if (ret == ERROR_SUCCESS) {
			return ret;
		};
	};

#if __SCALING__ >= SCALING_CC_NUMA
	/* If we're still here then there was no memory on the default bank. We
	 * have to now determine which bank would have memory, and set that to
	 * be the new default bank for raw contiguous allocations.
	 **/
	def = cur = numaStreams.prepareForLoop();
	currStream = (numaStreamC *)numaStreams.getLoopItem(&def);

	for (; currStream != __KNULL;
		currStream = (numaStreamC *)numaStreams.getLoopItem(&def))
	{
		// Attempt to allocate from the current stream.
		ret = currStream->memoryBank.contiguousGetFrames(nPages, paddr);

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

error_t numaTribC::fragmentedGetFrames(uarch_t nPages, paddr_t *paddr)
{
#if __SCALING__ >= SCALING_CC_NUMA
	numaBankId_t		def;
	numaBankId_t		cur;
	uarch_t			rwFlags;
#endif
	numaStreamC		*currStream;
	error_t			ret=0;

#if __SCALING__ >= SCALING_CC_NUMA
	defaultAffinity.def.lock.readAcquire(&rwFlags);
	def = defaultAffinity.def.rsrc;
	defaultAffinity.def.lock.readRelease(rwFlags);

	currStream = getStream(def);
#else
	//Now allocate from the default bank.
	currStream = getStream(defaultAffinity.def.rsrc);
#endif

	if (currStream != __KNULL)
	{
		ret = currStream->memoryBank.fragmentedGetFrames(nPages, paddr);

		if (ret > 0) {
			return ret;
		};
	};

#if __SCALING__ >= SCALING_CC_NUMA
	/* If we're still here then we failed at allocating from the default
	 * bank. Search each other bank, and get frames from one of them.
	 **/
	def = cur = numaStreams.prepareForLoop();
	currStream = (numaStreamC *)numaStreams.getLoopItem(&def);

	for (; currStream != __KNULL;
		currStream = (numaStreamC *)numaStreams.getLoopItem(&def))
	{
		ret = currStream->memoryBank.fragmentedGetFrames(nPages, paddr);

		if (ret > 0)
		{
			defaultAffinity.def.lock.writeAcquire();
			defaultAffinity.def.rsrc = cur;
			defaultAffinity.def.lock.writeRelease();
			return ret;
		};
		cur = def;
	};
#endif
	// If we're here, then we've failed.
	return ret;
}

// Preprocess out this whole function on a non-NUMA build.
#if __SCALING__ >= SCALING_CC_NUMA
error_t numaTribC::configuredGetFrames(
	localAffinityS *config, uarch_t nPages, paddr_t *paddr
	)
{
	numaBankId_t		def, cur;
	numaStreamC		*currStream;
	error_t			ret;
	uarch_t			rwFlags;
	// Get the thread's default config.
	config->def.lock.readAcquire(&rwFlags);
	def = config->def.rsrc;
	config->def.lock.readRelease(rwFlags);

	currStream = getStream(def);
	if (currStream != __KNULL)
	{
		ret = currStream->memoryBank.fragmentedGetFrames(nPages, paddr);

		if (ret > 0) {
			return ret;
		};
	};

	// Allocation from the default bank failed. Find a another default bank.
	def = cur = numaStreams.prepareForLoop();
	currStream = (numaStreamC *)numaStreams.getLoopItem(&def);

	for (; currStream != __KNULL;
		currStream = (numaStreamC *)numaStreams.getLoopItem(&def))
	{
		// If this bank is part of the thread's NUMA policy:
		if (config->memBanks.testSingle(cur))
		{
			ret = currStream->memoryBank
				.fragmentedGetFrames(nPages, paddr);

			if (ret > 0)
			{
				config->def.lock.writeAcquire();
				config->def.rsrc = cur;
				config->def.lock.writeRelease();
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

void numaTribC::mapRangeUsed(paddr_t baseAddr, uarch_t nPages)
{
#if __SCALING__ >= SCALING_CC_NUMA
	numaBankId_t	cur;
	numaStreamC	*currStream;
#endif


#if __SCALING__ >= SCALING_CC_NUMA
	cur = numaStreams.prepareForLoop();
	currStream = (numaStreamC *)numaStreams.getLoopItem(&cur);
	
	for (; currStream != __KNULL;
		currStream = (numaStreamC *)numaStreams.getLoopItem(&cur))
	{
		/* We can most likely afford this small speed bump since ranges
		 * of physical RAM are not often mapped or unmapped as used at
		 * runtime. This generally only happens when the kernel is made
		 * aware of an MMIO range.
		 **/
		currStream->memoryBank.mapMemUsed(baseAddr, nPages);
	};
#else
	getStream(defaultAffinity.def.rsrc)
		->memoryBank.mapMemUsed(baseAddr, nPages);
#endif
}

void numaTribC::mapRangeUnused(paddr_t baseAddr, uarch_t nPages)
{
#if __SCALING__ >= SCALING_CC_NUMA
	numaBankId_t	cur;
	numaStreamC	*currStream;
#endif


#if __SCALING__ >= SCALING_CC_NUMA
	cur = numaStreams.prepareForLoop();
	currStream = (numaStreamC *)numaStreams.getLoopItem(&cur);
	
	for (; currStream != __KNULL;
		currStream = (numaStreamC *)numaStreams.getLoopItem(&cur))
	{
		/* We can most likely afford this small speed bump since ranges
		 * of physical RAM are not often mapped or unmapped as used at
		 * runtime. This generally only happens when the kernel is made
		 * aware of an MMIO range.
		 **/
		currStream->memoryBank.mapMemUnused(baseAddr, nPages);
	};
#else
	getStream(defaultAffinity.def.rsrc)
		->memoryBank.mapMemUnused(baseAddr, nPages);
#endif
}

