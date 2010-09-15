
#include <__kstdlib/__kclib/assert.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>
#include <kernel/common/firmwareTrib/firmwareTrib.h>
#include <kernel/common/firmwareTrib/firmwareStream.h>
#include <kernel/common/numaTrib/numaTrib.h>

static void sortNumaMapByAddress(chipsetNumaMapS *map)
{
	numaMemMapEntryS	tmp;

	/** EXPLANATION:
	 * Simple one-pass swap sort algorithm. Recurses backward while sorting
	 * to avoid the need for multiple passes.
	 **/

	for (sarch_t i=0; i<static_cast<sarch_t>( map->nMemEntries - 1 ); )
	{		
		if (map->memEntries[i].baseAddr > map->memEntries[i+1].baseAddr)
		{
			memcpy(
				&tmp, &map->memEntries[i],
				sizeof(numaMemMapEntryS));

			memcpy(
				&map->memEntries[i], &map->memEntries[i+1],
				sizeof(numaMemMapEntryS));

			memcpy(
				&map->memEntries[i+1], &tmp,
				sizeof(numaMemMapEntryS));

			if (i != 0) { i--; };
			continue;
		};
		i++;
	};
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

#if __SCALING__ >= SCALING_CC_NUMA
void numaTribC::init2_spawnNumaStreams(chipsetNumaMapS *map)
{
	error_t		ret;

	for (uarch_t i=0; i<map->nMemEntries; i++)
	{
		// If a stream already exists for this bank:
		if (getStream(map->memEntries[i].bankId)) {
			continue;
		};

		// Else allocate a new stream.
		ret = spawnStream(map->memEntries[i].bankId);
		if (ret != ERROR_SUCCESS)
		{
			__kprintf(ERROR NUMATRIB"Failed to spawn stream for "
				"detected bank %d.\n",
				map->memEntries[i].bankId);

			continue;
		};

		__kprintf(NOTICE NUMATRIB"Spawned NUMA Stream for bank with ID "
			"%d.\n", map->memEntries[i].bankId);
	};
}
#endif

void numaTribC::init2_generateNumaMemoryRanges(chipsetNumaMapS *map)
{
	error_t		ret;
	numaStreamC	*ns;

	for (uarch_t i=0; i<map->nMemEntries; i++)
	{
		ns = getStream(map->memEntries[i].bankId);
		if (ns == __KNULL)
		{
			__kprintf(ERROR NUMATRIB"Bank %d found in NUMA map, "
				"but it has no stream in hw list.\n",
				map->memEntries[i].bankId);

			continue;
		};

		ret = ns->memoryBank.addMemoryRange(
			map->memEntries[i].baseAddr,
			map->memEntries[i].size);

		if (ret != ERROR_SUCCESS)
		{
			__kprintf(ERROR NUMATRIB"Failed to allocate "
				"memory range obj for range: base 0x%X "
				"size 0x%X on bank %d.\n",
				map->memEntries[i].baseAddr,
				map->memEntries[i].size,
				map->memEntries[i].bankId);
		};
	};
}

void numaTribC::init2_generateShbankFromNumaMap(
	chipsetMemConfigS *cfg, chipsetNumaMapS *map
	)
{
	error_t		ret;
	paddr_t			tmpBase, tmpSize;

	// NUMA map exists: need to discover holes for shbank.
	sortNumaMapByAddress(map);

	for (sarch_t i=0; i<static_cast<sarch_t>( map->nMemEntries ) - 1; i++)
	{
		/* Shbank is only for where holes intersect with memoryConfig.
		 * i.e., if memSize is reported to be 256MB, and there are 
		 * holes in the NUMA map higher up, those holes are ignored.
		 *
		 * Only holes that are below the memSize mark will get a shbank
		 * memory range.
		 **/
		if (map->memEntries[i].baseAddr > cfg->memSize) {
			break;
		};

		tmpBase = map->memEntries[i].baseAddr + map->memEntries[i].size;
		tmpSize = map->memEntries[i+1].baseAddr - tmpBase;

		if (tmpBase + tmpSize > cfg->memSize) {
			tmpSize = cfg->memSize - tmpBase;
		};

		if (tmpSize > 0)
		{
#ifdef CONFIG_DEBUG_NUMATRIB
			__kprintf(NOTICE NUMATRIB
				"For memrange %d, on bank %d, base 0x%X, size "
				"0x%X, next entry base 0x%X, shbank memory "
				"range with base 0x%X and size 0x%X is needed."
				"\n", i,
				map->memEntries[i].bankId,
				map->memEntries[i].baseAddr,
				map->memEntries[i].size,
				map->memEntries[i+1].baseAddr,
				tmpBase, tmpSize);
#endif

			ret = getStream(CHIPSET_MEMORY_NUMA_SHBANKID)
				->memoryBank.addMemoryRange(tmpBase, tmpSize);

			if (ret != ERROR_SUCCESS)
			{
				__kprintf(ERROR NUMATRIB
					"Shbank: Failed to spawn memrange for "
					"hole: base 0x%X size 0x%X.\n",
					tmpBase, tmpSize);
			}
			else
			{
				__kprintf(NOTICE NUMATRIB
					"Shbank: New memrange base 0x%X, size "
					"0x%X.\n",
					tmpBase, tmpSize);
			};
		};
	};
}

error_t numaTribC::initialize2(void)
{
	error_t			ret;
	chipsetMemConfigS	*memConfig=__KNULL;
	chipsetMemMapS		*memMap=__KNULL;
	chipsetNumaMapS		*numaMap=__KNULL;
	memInfoRivS		*memInfoRiv;
	numaStreamC		*ns;
	sarch_t			pos;
	status_t		nSet=0;

	// Initialize both firmware streams.
	ret = (*chipsetFwStream.initialize)();
	if (ret != ERROR_SUCCESS)
	{
		__kprintf(NOTICE NUMATRIB"Failed to init chipset FWStream.\n");
		return ret;
	};

	ret = (*firmwareFwStream.initialize)();
	if (ret != ERROR_SUCCESS)
	{
		__kprintf(NOTICE NUMATRIB"Failed to init firmware FWStream.\n");
		return ret;
	};

	// Fetch and initialize the Memory Info rivulet.
	memInfoRiv = firmwareTrib.getMemInfoRiv();
	if (ret != ERROR_SUCCESS)
	{
		__kprintf(NOTICE NUMATRIB"Chipset provides no Mem Info Riv.");
		return ERROR_UNKNOWN;
	};

	ret = (*memInfoRiv->initialize)();
	if (ret != ERROR_SUCCESS)
	{
		__kprintf(NOTICE NUMATRIB"Failed to init Mem Info Rivulet.\n");
		return ret;
	};

#if __SCALING__ >= SCALING_CC_NUMA
	// Get NUMA map from chipset.
	numaMap = (*memInfoRiv->getNumaMap)();

	if (numaMap != __KNULL && numaMap->nMemEntries > 0)
	{
		__kprintf(NOTICE NUMATRIB"Chipset NUMA Map: %d entries.\n",
			numaMap->nMemEntries);

		for (uarch_t i=0; i<numaMap->nMemEntries; i++)
		{
			__kprintf(NOTICE NUMATRIB"Entry %d: Base 0x%X, size "
				"0x%X, bank %d.\n",
				i,
				numaMap->memEntries[i].baseAddr,
				numaMap->memEntries[i].size,
				numaMap->memEntries[i].bankId);
		};

		init2_spawnNumaStreams(numaMap);
		init2_generateNumaMemoryRanges(numaMap);
	}
	else {
		__kprintf(WARNING NUMATRIB"getNumaMap(): No NUMA map.\n");
	};
#endif

#ifdef CHIPSET_MEMORY_NUMA_GENERATE_SHBANK
	// Get memory config from the chipset.
	memConfig = (*memInfoRiv->getMemoryConfig)();

	if (memConfig != __KNULL && memConfig->memSize > 0)
	{
		__kprintf(NOTICE NUMATRIB"Chipset Mem Config: memsize 0x%X.\n",
			memConfig->memSize);

		ret = spawnStream(CHIPSET_MEMORY_NUMA_SHBANKID);
		if (ret != ERROR_SUCCESS)
		{
			__kprintf(ERROR NUMATRIB"Failed to spawn shbank.\n");
			goto parseMemoryMap;
		};

		if (numaMap != __KNULL && numaMap->nMemEntries-1 > 0) {
			init2_generateShbankFromNumaMap(memConfig, numaMap);
		}
		else
		{

			ret = getStream(CHIPSET_MEMORY_NUMA_SHBANKID)
				->memoryBank.addMemoryRange(
					0x0, memConfig->memSize);

			if (ret != ERROR_SUCCESS)
			{
				__kprintf(ERROR NUMATRIB"Shbank: On shbank "
					"stream, failed to add shbank memrange."
					"\n");
			}
			else
			{
				__kprintf(NOTICE NUMATRIB"Shbank: no NUMA map. "
					"Spawn with total memsize 0x%X.\n",
					memConfig->memSize);
			};
		};
	}
	else {
		__kprintf(ERROR NUMATRIB"getMemoryConfig(): No mem config.\n");
	};
#endif

parseMemoryMap:
	// Get the Memory Map from the chipset code.
	memMap = (memInfoRiv->getMemoryMap)();

	if (memMap != __KNULL && memMap->nEntries > 0)
	{
		__kprintf(NOTICE NUMATRIB"Chipset Mem map: %d entries.\n",
			memMap->nEntries);

		for (uarch_t i=0; i<memMap->nEntries; i++)
		{
			__kprintf(NOTICE NUMATRIB"Entry %d: Base 0x%X, size "
				"0x%X, type %d.\n",
				i,
				memMap->entries[i].baseAddr,
				memMap->entries[i].size,
				memMap->entries[i].memType);
		};

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

	// Next merge all banks with __kspace.
	pos = numaStreams.prepareForLoop();
	ns = numaStreams.getLoopItem(&pos);
	for (; ns != __KNULL; ns = numaStreams.getLoopItem(&pos)) {
		nSet = ns->memoryBank.merge(
			&getStream(CHIPSET_MEMORY_NUMA___KSPACE_BANKID)
				->memoryBank);
	};

	__kprintf(NOTICE NUMATRIB"%d frames were merged from __kspace into new "
		"PMM state.\n", nSet);

	// Then apply the Memory Tributary's Memory Regions to all banks.
	if (chipsetRegionMap != __KNULL)
	{
		pos = numaStreams.prepareForLoop();
		ns = numaStreams.getLoopItem(&pos);
		for (; ns != __KNULL; ns = numaStreams.getLoopItem(&pos))
		{
			for (uarch_t i=0; i<chipsetRegionMap->nEntries; i++)
			{
				ns->memoryBank.mapMemUsed(
					chipsetRegionMap->entries[i].baseAddr,
					PAGING_BYTES_TO_PAGES(
						chipsetRegionMap
							->entries[i].size));
			};
		};
	};

	return ERROR_SUCCESS;
}

