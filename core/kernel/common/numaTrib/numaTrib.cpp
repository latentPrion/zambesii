
#include <scaling.h>
#include <arch/paging.h>
#include <chipset/__kmemory.h>
#include <chipset/numaMap.h>
#include <chipset/memoryMap.h>
#include <chipset/memoryConfig.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kstdlib/__kclib/string.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/numaTrib/numaTrib.h>

/**	EXPLANATION:
 * To initialize the NUMA Tributary, the steps are:
 *	1. Pre-allocate room for a single pointer to point to the __kspace
 *	   fake stream at boot and point the array of NUMA stream pointers to
 *	   point to that pointer.
 *	2. Pre-allocate an actual instance of numaStreamC for the __kspace
 *	   NUMA stream and have it constructed with the chipset's __kspace
 *	   mem range.
 *	3. Make the pre-allocated pointer in (1) point to the pre-allocated
 *	   stream in (2).
 *	4. Call initialize() on the pre-allocated stream from (2), with the
 *	   pre-allocated memory for __kspace passed to it as an argument.
 *	5. Set numaStreams.rsrc.nStreams to 1.
 *	6. Done. We now have a fake NUMA bank at index 0 which allocates from
 *	   a guaranteed usable area of physical memory for bootup.
 **/

#define NUMATRIB		"Numa Tributary: "

// Always initialize the __kspace stream as a fake bank 0.
static numaStreamC	__kspaceNumaStream(0);
static numaStreamC	*__kspaceStreamPtr;
// Space for the numaMemoryBank's array.
static numaMemoryRangeC	*__kspaceMemoryRangePtr;
static numaMemoryRangeC	__kspaceMemoryRange(
	CHIPSET_MEMORY___KSPACE_BASE,
	CHIPSET_MEMORY___KSPACE_SIZE);


numaTribC::numaTribC(void)
{
#if __SCALING__ >= SCALING_CC_NUMA
	defaultConfig.def.rsrc = 0;
#endif
	numaStreams.rsrc.arr = __KNULL;
	numaStreams.rsrc.nStreams = 0;

	sharedBank = NUMATRIB_SHBANK_INVALID;
}

error_t numaTribC::initialize(void)
{
	error_t		ret;

	numaStreams.rsrc.arr = &__kspaceStreamPtr;
	numaStreams.rsrc.arr[0] = &__kspaceNumaStream;
	numaStreams.rsrc.nStreams = 1;

	ret = numaStreams.rsrc.arr[0]->memoryBank.__kspaceAddMemoryRange(
		&__kspaceMemoryRangePtr,
		&__kspaceMemoryRange,
		__kspaceInitMem);

	return ret;
}

/**	EXPLANATION:
 * Initialize2(): Detects physical memory on the chipset using firmware services
 * contained in the Firmware Tributary. If the person porting the kernel to
 * the current chipset did not provide a firmware interface, the kernel will
 * simply live on with only the __kspace bank; Of course, it should be obvious
 * that this is probably sub-optimal, but it can work.
 *
 * Generally, the kernel relies on the memInfoRiv to provide all memory
 * information. A prime example of all of this is the IBM-PC. For the IBM-PC,
 * the memInfoRiv is actually just a wrapper around the x86Emu library. x86Emu
 * Completely unbeknownst to the kernel, the IBM-PC support code initializes and
 * runs a full real mode emulator for memory detection. For NUMA detection, the
 * chipset firmware rivulet code will map low memory into the kernel's address
 * space and scan for the ACPI tables, trying to find NUMA information in the
 * SRAT/SLIT tables.
 *
 * To actually get the pmem information and numa memory layout information into
 * a usable state, the kernel must spawn a NUMA Stream for each detected bank,
 * and initialize it real-time.
 *
 * In the broader scope, there are two possibilities:
 *	1. The kernel finds and parses a physical memory information structure
 *	   or memory map before discovering NUMA memory layout.
 *	2. The kernel finds NUMA layout information before finding any kind of
 *	   general memory information or memory map.
 *
 * In Zambezii, a memory map is nothing more than something to overlay the
 * the real memory information. In other words, the last thing we logically
 * parse is a memory map. Our priority is:
 *	1. Find out the total amount of memory first.
 *	2. Find out about NUMA layout next. See, doing it this way allows us to
 *	   discover the existence of any memory on-board which does not belong
 *	   to a particular NUMA bank.
 *
 *	   Take the following example: If a chipset is detailed to have 64MB of
 *	   RAM, yet the NUMA information describes only two nodes: (1) 0MB-8MB,
 *	   (2) 20MB-32MB, with a hole between 8MB and 20MB, and another hole
 *	   between 32MB and 64MB, we will spawn a third and fourth bank for the
 *	   two banks which were not described explicitly as NUMA banks, and
 *	   treat them as local memory to all NUMA banks. That is, we'll treat
 *	   those undescribed memory ranges as shared memory that is globally
 *	   the same distance from each node.
 *
 *	3. Find a memory map. When we know how much RAM there is, and also the
 *	   NUMA layout of this RAM, we can then pass through a memory map and
 *	   apply the information in the memory map to the NUMA banks. That is,
 *	   mark all reserved ranges as 'used' in the PMM info, and whatnot.
 *	   Note well that a memory map may be used as general memory information
 *	   suitable for use as requirement (1) above.
 *
 * ^ If NUMA information does not exist, Zambezii will spawn a single NUMA bank
 *   as if the chipset only had one bank, and all processes will share this
 *   bank.
 *
 * ^ If a memory map is not found, and only a total figure for "amount of RAM"
 *   is given, Zambezii will assume that there is no reserved memory on the
 *   chipset and operate as if all RAM is available for use.
 *
 * ^ In the absence of a total figure for "amount of RAM", (where this figure
 *   may be provided explicitly, or derived from a memory map), Zambezii will
 *   simply assume that the only usable RAM is the __kspace RAM (bootmem), and
 *   continue to use that. When that runs out, that's that.
 **/
error_t numaTribC::initialize2(void)
{
/*	chipsetMemConfigS	*memConfig;
	chipsetMemMapS		*memMap;
	chipsetNumaMapS		*numaMap;
*/
	return ERROR_SUCCESS;
}

void numaTribC::dump(void)
{
	uarch_t		rwFlags, nStreams;

	numaStreams.lock.readAcquire(&rwFlags);
	nStreams = numaStreams.rsrc.nStreams;
	numaStreams.lock.readRelease(rwFlags);

	__kprintf(NOTICE NUMATRIB"Dumping. nStreams %d.\n", nStreams);

	for (uarch_t i=0; i<nStreams; i++) {
		getStream(i)->memoryBank.dump();
	}
}

error_t numaTribC::spawnStream(numaBankId_t, paddr_t, paddr_t)
{
	UNIMPLEMENTED("numaTribC::spawnStream()");
	return ERROR_UNIMPLEMENTED;
}

numaTribC::~numaTribC(void)
{
	if (numaStreams.rsrc.arr != __KNULL)
	{
		for (; numaStreams.rsrc.nStreams > 0;
			numaStreams.rsrc.nStreams--)
		{
			if (numaStreams.rsrc.arr[numaStreams.rsrc.nStreams]
				!= __KNULL)
			{
				delete numaStreams.rsrc.arr[
					numaStreams.rsrc.nStreams];
			};
		};
		delete numaStreams.rsrc.arr;
	};
}

#if __SCALING__ >= SCALING_CC_NUMA
numaStreamC *numaTribC::getStream(numaBankId_t bankId)
{
	numaStreamC		*ret;
	uarch_t			rwFlags;

	numaStreams.lock.readAcquire(&rwFlags);

	ret = numaStreams.rsrc.arr[bankId];

	numaStreams.lock.readRelease(rwFlags);
	return ret;
}
#endif

void numaTribC::releaseFrames(paddr_t paddr, uarch_t nFrames)
{
	numaStreamC	*currStream;
	uarch_t		nStreams, rwFlags;

	/**	EXPLANATION:
	 * Here we have the result of a trade-off. For a reduction in the size
	 * of each entry in allocTableC and better design overall, and smaller
	 * memory footprint, we traded off speed on memory frees.
	 *
	 * The kernel now has to decipher which bank a range of pmem belongs to
	 * before it can be freed.
	 **/
	numaStreams.lock.readAcquire(&rwFlags); 
	nStreams = numaStreams.rsrc.nStreams;
	numaStreams.lock.readRelease(rwFlags);

#if __SCALING__ >= SCALING_CC_NUMA
	for (uarch_t i=0; i<nStreams; i++)
	{
		currStream = getStream(i);

		// Ensure we're not trying to free to hot-swapped out, etc RAM.
		if (currStream != __KNULL)
		{
			if (currStream->memoryBank.identifyPaddr(paddr))
			{
				currStream->memoryBank.releaseFrames(
					paddr, nFrames);
				return;
			};
		};
	};
	/* Couldn't find a suitable bank. Probably the memory was hot swapped,
	 * or there's corruption in the memory manager somewhere.
	 **/
	__kprintf(WARNING NUMATRIB"releaseFrames(0x%X, %d): pmem leak.\n",
		paddr, nFrames);
#else
	numaStreams.rsrc.arr[0]->memoryBank.releaseFrames(paddr, nFrames);
#endif

}

error_t numaTribC::contiguousGetFrames(uarch_t nPages, paddr_t *paddr)
{
	numaBankId_t		def;
	uarch_t			rwFlags, nStreams;
	numaStreamC		*currStream;
	error_t			ret;

#if __SCALING__ >= SCALING_CC_NUMA
	// Get the default bank's Id.
	defaultConfig.def.lock.readAcquire(&rwFlags);

	def = defaultConfig.def.rsrc;

	defaultConfig.def.lock.readRelease(rwFlags);
#else
	// For a non-NUMA build, the kernel just fakes a bank with id=0.
	def = 0;
#endif

	// Allocate from the default bank.
	currStream = getStream(def);
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
	numaStreams.lock.readAcquire(&rwFlags);
	nStreams = numaStreams.rsrc.nStreams;
	numaStreams.lock.readRelease(rwFlags);

	for (ubit32 i=0; i<nStreams; i++)
	{
		currStream = getStream(i);
		if (currStream != __KNULL)
		{
			// Attempt to allocate from the current stream.
			ret = currStream->memoryBank.contiguousGetFrames(
				nPages, paddr);

			if (ret == ERROR_SUCCESS)
			{
				// Set the current stream to be the new default.
				defaultConfig.def.lock.writeAcquire();

				defaultConfig.def.rsrc = i;

				defaultConfig.def.lock.writeRelease();
				// Return. We got memory off the current bank.
				return ret;
			};
		};
	};
#endif
	// If we reached here, then we've failed completely.
	return ERROR_MEMORY_NOMEM_PHYSICAL;
}

error_t numaTribC::fragmentedGetFrames(uarch_t nPages, paddr_t *paddr)
{
	error_t			ret=0;
	numaBankId_t		def;
	uarch_t			rwFlags, nStreams;
	numaStreamC		*currStream;

#if __SCALING__ >= SCALING_CC_NUMA
	defaultConfig.def.lock.readAcquire(&rwFlags);

	def = defaultConfig.def.rsrc;

	defaultConfig.def.lock.readRelease(rwFlags);
#else
	def = 0;
#endif

	//Now allocate from the default bank.
	currStream = getStream(def);
	if (currStream != __KNULL)
	{
		ret = currStream->memoryBank.fragmentedGetFrames(
			nPages, paddr);

		if (ret > 0) {
			return ret;
		};
	};

#if __SCALING__ >= SCALING_CC_NUMA
	/* If we're still here then we failed at allocating from the default
	 * bank. Search each other bank, and get frames from one of them.
	 **/
	numaStreams.lock.readAcquire(&rwFlags);
	nStreams = numaStreams.rsrc.nStreams;
	numaStreams.lock.readRelease(rwFlags);

	for (ubit32 i=0; i<nStreams; i++)
	{
		currStream = getStream(i);
		if (currStream != __KNULL)
		{
			ret = currStream->memoryBank.fragmentedGetFrames(
				nPages, paddr);

			if (ret > 0)
			{
				defaultConfig.def.lock.writeAcquire();

				defaultConfig.def.rsrc = i;

				defaultConfig.def.lock.writeRelease();
				return ret;
			};
		};
	};
#endif
	// If we're here, then we've failed.
	return ret;
}

// Preprocess out this whole function on a non-NUMA build.
#if __SCALING__ >= SCALING_CC_NUMA
error_t numaTribC::configuredGetFrames(
	numaConfigS *config, uarch_t nPages, paddr_t *paddr
	)
{
	uarch_t			rwFlags, nStreams;
	numaBankId_t		def;
	numaStreamC		*currStream;
	error_t			ret;

	// Get the thread's default config.
	config->def.lock.readAcquire(&rwFlags);

	def = config->def.rsrc;

	config->def.lock.readRelease(rwFlags);

	currStream = getStream(def);
	if (currStream != __KNULL)
	{
		ret = currStream->memoryBank.fragmentedGetFrames(
			nPages, paddr);

		if (ret > 0) {
			return ret;
		};
	};

	// Allocation from the default bank failed. Find a another default bank.
	numaStreams.lock.readAcquire(&rwFlags);
	nStreams = numaStreams.rsrc.nStreams;
	numaStreams.lock.readRelease(rwFlags);

	for (ubit32 i=0; i<nStreams; i++)
	{
		// If this bank is part of the thread's NUMA policy:
		if (config->memBanks.testSingle(i))
		{
			currStream = getStream(i);
			if (currStream != __KNULL)
			{
				ret = currStream->memoryBank
					.fragmentedGetFrames(nPages, paddr);

				if (ret > 0)
				{
					config->def.lock.writeAcquire();
					
					config->def.rsrc = i;
					
					config->def.lock.writeRelease();
					return ret;
				};
			};
		};
	};

	/* If we reach here then none of the thread's configured banks has any
	 * physical mem left on it.
	 **/
	return ERROR_MEMORY_NOMEM_PHYSICAL;
}
#endif

void numaTribC::mapRangeUsed(paddr_t baseAddr, uarch_t nPages)
{
	uarch_t		rwFlags, nStreams;
	numaStreamC	*currStream;

	numaStreams.lock.readAcquire(&rwFlags);
	nStreams = numaStreams.rsrc.nStreams;
	numaStreams.lock.readRelease(rwFlags);

	for (uarch_t i=0; i<numaStreams.rsrc.nStreams; i++)
	{
		currStream = getStream(i);

		/* We can most likely afford this small speed bump since ranges
		 * of physical RAM are not often mapped or unmapped as used at
		 * runtime. This generally only happens when the kernel is made
		 * aware of an MMIO range.
		 **/
		currStream->memoryBank.mapMemUsed(baseAddr, nPages);
	};
}

void numaTribC::mapRangeUnused(paddr_t baseAddr, uarch_t nPages)
{
	uarch_t		rwFlags, nStreams;
	numaStreamC	*currStream;

	numaStreams.lock.readAcquire(&rwFlags);
	nStreams = numaStreams.rsrc.nStreams;
	numaStreams.lock.readRelease(rwFlags);

	for (uarch_t i=0; i<numaStreams.rsrc.nStreams; i++)
	{
		currStream = getStream(i);
		currStream->memoryBank.mapMemUnused(baseAddr, nPages);
	};
}

