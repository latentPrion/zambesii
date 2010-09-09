
#include <scaling.h>
#include <arch/paging.h>
#include <chipset/memory.h>
#include <chipset/numaMap.h>
#include <chipset/memoryMap.h>
#include <chipset/memoryConfig.h>
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

#define NUMATRIB		"Numa Tributary: "

// Initialize the __kspace NUMA Stream to its configured Stream ID.
static numaStreamC	__kspaceNumaStream(CHIPSET_MEMORY_NUMA___KSPACE_BANKID);
// Space for the numaMemoryBank's array.
static numaMemoryBankC::rangePtrS	__kspaceRangePtrMem;
static numaMemoryRangeC			__kspaceMemoryRange(
	CHIPSET_MEMORY___KSPACE_BASE,
	CHIPSET_MEMORY___KSPACE_SIZE);


numaTribC::numaTribC(void)
{
	defaultConfig.def.rsrc = CHIPSET_MEMORY_NUMA___KSPACE_BANKID;
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
 * In Zambezii, a memory map is nothing more than something to overlay the
 * the real memory information. In other words, the last thing we logically
 * parse is a memory map. Our priority is:
 *	1. Find out about NUMA layout.
 *	2. Find out the total amount of memory.
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
 * ^ If NUMA information does not exist, and shared bank generation is not
 *   set in the config, Zambezii will assume no memory other than __kspace. If
 *   shbank is configured, then Zambezii will spawn a single bank for all of
 *   RAM as shared memory for all processes.
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
	error_t			ret;
	chipsetMemConfigS	*memConfig;
	chipsetMemMapS		*memMap;
	chipsetNumaMapS		*numaMap;
	memInfoRivS		*memInfoRiv;
	numaStreamC		*ns;
	sarch_t			pos;

	/**	EXPLANATION:
	 * Now the NUMA Tributary is ready to check for new banks of memory,
	 * or for a non-NUMa build, generate a single "shared" NUMA bank as a
	 * fake bank 0 from which all threads will allocate.
	 *
	 * On both a NUMA and a non-NUMA build, the kernel will eradicate
	 * __kspace at this point. On a non-NUMA build however, if the kernel
	 * finds no extra memory, it will send out a nice, conspicuous warning
	 * and then assign __kspace to be the shared bank, and move on.
	 *
	 * On a NUMA build, if the kernel finds no NUMA memory config, then it
	 * will fall through to the check for shared memory. However, if the
	 * person porting the kernel to his or her chipset did not define
	 * CHIPSET_MEMORY_NUMA_GENERATE_SHBANK, then the kernel will just
	 * fall through, see that __kspace is the only remaining memory, and
	 * set that as the default bank, send out a nice highly conspicuous
	 * warning, and move on.
	 *
	 * In the event that CHIPSET_MEMORY_NUMA_GENERATE_SHBANK is defined,
	 * yet still no memory is found, the kernel will do the same as above.
	 *
	 * In the event that CHIPSET_MEMORY_NUMA_GENERATE_SHBANK is defined,
	 * and memory is found, the kernel will set that memory to be shbank,
	 * and the default bank, and then eradicate __kspace.
	 **/
	// Initialize both firmware streams.
	ret = (*chipsetFwStream.initialize)();
	assert_fatal(ret == ERROR_SUCCESS);

	ret = (*firmwareFwStream.initialize)();
	assert_fatal(ret == ERROR_SUCCESS);

	__kprintf(NOTICE NUMATRIB"Initialized Firmware and Chipset firmware "
		"streams.\n");

	// Fetch and initialize the Memory Info rivulet.
	memInfoRiv = firmwareTrib.getMemInfoRiv();
	assert_fatal(memInfoRiv != __KNULL);

	ret = (*memInfoRiv->initialize)();
	assert_fatal(ret == ERROR_SUCCESS);

#if __SCALING__ >= SCALING_CC_NUMA
	numaMap = (*memInfoRiv->getNumaMap)();
	if (numaMap != __KNULL)
	{
		__kprintf(NOTICE NUMATRIB"NUMA Map: %d entries.\n",
			numaMap->nMemEntries);

		for (uarch_t i=0; i<numaMap->nMemEntries; i++)
		{
			// If we've already spawned a stream for this bank:
			if (getStream(numaMap->memEntries[i].bankId)) {
				continue;
			};
			// Else allocate one.
			ns = new (
				(memoryTrib.__kmemoryStream
					.*memoryTrib.__kmemoryStream.memAlloc)(
						PAGING_BYTES_TO_PAGES(
							sizeof(numaStreamC)),
						MEMALLOC_NO_FAKEMAP))
				numaStreamC(numaMap->memEntries[i].bankId);

			if (ns == __KNULL)
			{
				__kprintf(ERROR NUMATRIB"Failed to allocate "
					"Numa Stream obj for bank %d.\n",
					numaMap->memEntries[i].bankId);
			}
			else
			{
				ret = numaStreams.addItem(
					numaMap->memEntries[i].bankId, ns);

				if (ret != ERROR_SUCCESS)
				{
					__kprintf(ERROR NUMATRIB"Failed to "
						"add NUMA Stream for bank %d "
						"to NUMA Trib list.\n",
						numaMap->memEntries[i].bankId);

					memoryTrib.__kmemoryStream.memFree(ns);
				}
				else
				{
					__kprintf(NOTICE NUMATRIB"New NUMA "
						"Stream obj, ID %d, v 0x%X.\n",
						numaMap->memEntries[i].bankId,
						ns);
				};
			};
		};
	}
	else {
		__kprintf(WARNING NUMATRIB"getNumaMap(): no map.\n");
	};
#endif

#ifdef CHIPSET_MEMORY_NUMA_GENERATE_SHBANK
	memConfig = (*memInfoRiv->getMemoryConfig)();
	if (memConfig != __KNULL)
	{
		__kprintf(NOTICE NUMATRIB"Memory Config: memory size: 0x%X.\n",
			memConfig->memSize);

		ns = new (
			(memoryTrib.__kmemoryStream
				.*memoryTrib.__kmemoryStream.memAlloc)(
					PAGING_BYTES_TO_PAGES(
						sizeof(numaStreamC)),
					MEMALLOC_NO_FAKEMAP))
			numaStreamC(CHIPSET_MEMORY_NUMA_SHBANKID);

		ret = numaStreams.addItem(CHIPSET_MEMORY_NUMA_SHBANKID, ns);
		if (ret != ERROR_SUCCESS)
		{
			__kprintf(ERROR NUMATRIB"Failed to add shbank to NUMA "
				"Stream list.\n");
		}
		else
		{
			__kprintf(NOTICE NUMATRIB"Shbank added @ index %d.\n",
				CHIPSET_MEMORY_NUMA_SHBANKID);

			ns = getStream(CHIPSET_MEMORY_NUMA_SHBANKID);
			if (ns != __KNULL)
			{
				ret = ns->memoryBank.addMemoryRange(
					0x0, memConfig->memSize);

				if (ret != ERROR_SUCCESS)
				{
					__kprintf(ERROR NUMATRIB"Failed to add "
						"memory range for shbank.\n");
				}
				else
				{
					__kprintf(NOTICE NUMATRIB"Shbank mem "
						"range added to shbank.\n");
				};
			}
			else
			{
				__kprintf(ERROR NUMATRIB"Failed to retrieve "
					"shbank stream pointer from list.\n");

				panic(ERROR_UNKNOWN);
			};
		};
	}
	else {
		__kprintf(WARNING NUMATRIB"getMemoryConfig(): no config.\n");
	};
#endif

	memMap = (memInfoRiv->getMemoryMap)();
	if (memMap != __KNULL)
	{
		pos = numaStreams.prepareForLoop();
		ns = numaStreams.getLoopItem(&pos);
		for (; ns != __KNULL; ns = numaStreams.getLoopItem(&pos))
		{
			for (uarch_t i=0; i<memMap->nEntries; i++)
			{
				if (memMap->entries[i].memType !=
					CHIPSETMMAP_TYPE_USABLE)
				{
					ns->memoryBank.mapMemUsed(
						memMap->entries[i].baseAddr,
						PAGING_BYTES_TO_PAGES(
							memMap->entries[i]
								.size));
				};
			};
		};
	}
	else {
		__kprintf(WARNING NUMATRIB"getMemoryMap(): No mem map.\n");
	};

	return ERROR_SUCCESS;
}

void numaTribC::dump(void)
{
	numaBankId_t	cur;
	numaStreamC	*curStream;

	__kprintf(NOTICE NUMATRIB"Dumping. nStreams %d.\n", nStreams);

	cur = numaStreams.prepareForLoop();
	curStream = numaStreams.getLoopItem(&cur);

	for (; curStream != __KNULL; curStream=numaStreams.getLoopItem(&cur)) {
		curStream->memoryBank.dump();
	}
}

error_t numaTribC::spawnStream(numaBankId_t)
{
	UNIMPLEMENTED("numaTribC::spawnStream()");
	return ERROR_UNIMPLEMENTED;
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
	currStream = numaStreams.getLoopItem(&cur);

	for (; currStream != __KNULL;
		currStream = numaStreams.getLoopItem(&cur))
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
	getStream(defaultConfig.def.rsrc)
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
	defaultConfig.def.lock.readAcquire(&rwFlags);
	def = defaultConfig.def.rsrc;
	defaultConfig.def.lock.readRelease(rwFlags);

	currStream = getStream(def);	
#else
	// Allocate from default bank, which is the sharedBank for non-NUMA.
	currStream = getStream(defaultConfig.def.rsrc);
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
	currStream = numaStreams.getLoopItem(&def);

	for (; currStream != __KNULL;
		currStream = numaStreams.getLoopItem(&def))
	{
		// Attempt to allocate from the current stream.
		ret = currStream->memoryBank.contiguousGetFrames(nPages, paddr);

		if (ret == ERROR_SUCCESS)
		{
			// Set the current stream to be the new default.
			defaultConfig.def.lock.writeAcquire();

			defaultConfig.def.rsrc = cur;

			defaultConfig.def.lock.writeRelease();
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
	defaultConfig.def.lock.readAcquire(&rwFlags);
	def = defaultConfig.def.rsrc;
	defaultConfig.def.lock.readRelease(rwFlags);

	currStream = getStream(def);
#else
	//Now allocate from the default bank.
	currStream = getStream(defaultConfig.def.rsrc);
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
	currStream = numaStreams.getLoopItem(&def);

	for (; currStream != __KNULL;
		currStream = numaStreams.getLoopItem(&def))
	{
		ret = currStream->memoryBank.fragmentedGetFrames(nPages, paddr);

		if (ret > 0)
		{
			defaultConfig.def.lock.writeAcquire();

			defaultConfig.def.rsrc = cur;

			defaultConfig.def.lock.writeRelease();
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
	numaConfigS *config, uarch_t nPages, paddr_t *paddr
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
	currStream = numaStreams.getLoopItem(&def);

	for (; currStream != __KNULL;
		currStream = numaStreams.getLoopItem(&def))
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
	currStream = numaStreams.getLoopItem(&cur);
	
	for (; currStream != __KNULL;
		currStream = numaStreams.getLoopItem(&cur))
	{
		/* We can most likely afford this small speed bump since ranges
		 * of physical RAM are not often mapped or unmapped as used at
		 * runtime. This generally only happens when the kernel is made
		 * aware of an MMIO range.
		 **/
		currStream->memoryBank.mapMemUsed(baseAddr, nPages);
	};
#else
	getStream(defaultConfig.def.rsrc)
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
	currStream = numaStreams.getLoopItem(&cur);
	
	for (; currStream != __KNULL;
		currStream = numaStreams.getLoopItem(&cur))
	{
		/* We can most likely afford this small speed bump since ranges
		 * of physical RAM are not often mapped or unmapped as used at
		 * runtime. This generally only happens when the kernel is made
		 * aware of an MMIO range.
		 **/
		currStream->memoryBank.mapMemUnused(baseAddr, nPages);
	};
#else
	getStream(defaultConfig.def.rsrc)
		->memoryBank.mapMemUnused(baseAddr, nPages);
#endif
}

