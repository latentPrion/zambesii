
#include <chipset/regionMap.h>
#include <chipset/pkg/chipsetPackage.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>
#include <kernel/common/numaMemoryBank.h>
#include <kernel/common/numaTrib/numaTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/memoryTrib/memoryTrib.h>

/**	EXPLANATION:
 * The Zambezii NUMA Tributary's initialize2() sequence. Its purpose is to
 * detect physical memory on a chipset.
 *
 * The kernel does this in several distinct stages:
 *	1. Parse a NUMA map with details on NUMA memory layout.
 *		a. Generate a NUMA Stream for each detected bank.
 *		b. Generate a memory range obj for each detected memory range.
 *	2. Retrieve a Memory Config structure from the chipset.
 *	3. Generate the Shared NUMA Bank (aka shbank).
 *		-> If there is a NUMA map from (1) above:
 *			a. For each NUMA memory bank, see if there exists any
 *			   hole between any of its given NUMA memory ranges. If
 *			   so, generate a memory range for the hole and put it
 *			   into the shared NUMA bank, such that the kernel will
 *			   treat it as if it's shared memory with the same
 *			   access latency from all nodes.
 *		-> If not:
 *			a. Generate one big memory range to cover all of memory
 *			   and treat it as one shared memory bank with the same
 *			   latency across all nodes.
 *	4. Retrieve a memory map from the chipset.
 *	5. Overlay the memory type information in the memory map onto all
 *	   the NUMA banks in the system.
 *	6. Overlay the set apart memory ranges in Memory Trib with the memory
 *	   map's memory type information.
 *	7. Overlay all memory banks with __kspace, merging all banks with the
 *	   alloc info contained in __kspace. This essentially marks the end of
 *	   the need for __kspace.
 *
 *	8. If __kspace is now not the only bank in the NUMA Tributary, we can
 *	   get rid of it.
 **/

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
			memcpy8(
				&tmp, &map->memEntries[i],
				sizeof(numaMemMapEntryS));

			memcpy8(
				&map->memEntries[i], &map->memEntries[i+1],
				sizeof(numaMemMapEntryS));

			memcpy8(
				&map->memEntries[i+1], &tmp,
				sizeof(numaMemMapEntryS));

			if (i != 0) { i--; };
			continue;
		};
		i++;
	};
}

error_t memoryTribC::pmemInit(void)
{
	error_t			ret;
	chipsetMemConfigS	*memConfig=__KNULL;
	chipsetMemMapS		*memMap=__KNULL;
	chipsetNumaMapS		*numaMap=__KNULL;
	memoryModS		*memoryMod;
	numaMemoryBankC		*nmb;
	// __kspaceBool is used to determine whether or not to kill __kspace.
	sarch_t			pos, prevPos, __kspaceBool=0;
	status_t		nSet=0;

	if (chipsetPkg.initialize == __KNULL || chipsetPkg.memory == __KNULL)
	{
		__kprintf(ERROR"Missing chipsetPkg.initialize(), or Memory \
			Info module.\n");

		return ERROR_FATAL;
	};

	// Initialize both firmware streams.
	ret = (*chipsetPkg.initialize)();
	if (ret != ERROR_SUCCESS)
	{
		__kprintf(NOTICE MEMTRIB"Failed to init chipset package.\n");
		return ret;
	};

	// Fetch and initialize the Memory Info Module.
	memoryMod = chipsetPkg.memory;
	ret = (*memoryMod->initialize)();
	if (ret != ERROR_SUCCESS)
	{
		__kprintf(NOTICE MEMTRIB"Failed to init memory info module.\n");
		return ret;
	};

#if __SCALING__ >= SCALING_CC_NUMA
	// Get NUMA map from chipset.
	numaMap = (*memoryMod->getNumaMap)();

	if (numaMap != __KNULL && numaMap->nMemEntries > 0)
	{
		__kprintf(NOTICE MEMTRIB"Chipset NUMA Map: %d entries.\n",
			numaMap->nMemEntries);

		for (uarch_t i=0; i<numaMap->nMemEntries; i++)
		{
			__kprintf(NOTICE MEMTRIB"Entry %d: Base 0x%P, size "
				"0x%P, bank %d.\n",
				i,
				numaMap->memEntries[i].baseAddr,
				numaMap->memEntries[i].size,
				numaMap->memEntries[i].bankId);
		};

		init2_spawnNumaStreams(numaMap);
		init2_generateNumaMemoryRanges(numaMap, &__kspaceBool);
	}
	else {
		__kprintf(WARNING MEMTRIB"getNumaMap(): No NUMA map.\n");
	};
#endif

#ifdef CHIPSET_MEMORY_NUMA_GENERATE_SHBANK
	// Get memory config from the chipset.
	memConfig = (*memoryMod->getMemoryConfig)();

	if (memConfig != __KNULL && memConfig->memSize > 0)
	{
		__kprintf(NOTICE MEMTRIB"Chipset Mem Config: memsize 0x%P.\n",
			memConfig->memSize);

		ret = createBank(CHIPSET_MEMORY_NUMA_SHBANKID);
		if (ret != ERROR_SUCCESS)
		{
			__kprintf(ERROR MEMTRIB"Failed to spawn shbank on "
				"Memory Trib.\n");

			goto parseMemoryMap;
		};

		if (numaMap != __KNULL && numaMap->nMemEntries-1 > 0)
		{
			init2_generateShbankFromNumaMap(
				memConfig, numaMap, &__kspaceBool);
		}
		else
		{

			ret = getBank(CHIPSET_MEMORY_NUMA_SHBANKID)
				->addMemoryRange(
					memConfig->memBase, memConfig->memSize);

			if (ret != ERROR_SUCCESS)
			{
				__kprintf(ERROR MEMTRIB"Shbank: On shbank "
					"obj, failed to add shbank memrange."
					"\n");
			}
			else
			{
				__kprintf(NOTICE MEMTRIB"Shbank: no NUMA map. "
					"Spawn with total memsize 0x%P.\n",
					memConfig->memSize);

				__kspaceBool = 1;
			};
		};
	}
	else {
		__kprintf(ERROR MEMTRIB"getMemoryConfig(): No mem config.\n");
	};
#endif

parseMemoryMap:
	// Get the Memory Map from the chipset code.
	memMap = (*memoryMod->getMemoryMap)();

	if (memMap != __KNULL && memMap->nEntries > 0)
	{
		__kprintf(NOTICE MEMTRIB"Chipset Mem map: %d entries.\n",
			memMap->nEntries);

		for (uarch_t i=0; i<memMap->nEntries; i++)
		{
			__kprintf(NOTICE MEMTRIB"Entry %d: Base 0x%P, size "
				"0x%P, type %d.\n",
				i,
				memMap->entries[i].baseAddr,
				memMap->entries[i].size,
				memMap->entries[i].memType);
		};

		pos = memoryBanks.prepareForLoop();
		nmb = (numaMemoryBankC *)memoryBanks.getLoopItem(&pos);
		for (; nmb != __KNULL;
			nmb = (numaMemoryBankC *)memoryBanks.getLoopItem(&pos))
		{
			for (uarch_t i=0; i<memMap->nEntries; i++)
			{
				if (memMap->entries[i].memType !=
					CHIPSETMMAP_TYPE_USABLE)
				{
					nmb->mapMemUsed(
						memMap->entries[i].baseAddr,
						PAGING_BYTES_TO_PAGES(
							memMap->entries[i]
								.size));
				};
			};
		};
	}
	else {
		__kprintf(WARNING MEMTRIB"getMemoryMap(): No mem map.\n");
	};

	// Next merge all banks with __kspace.
	pos = memoryBanks.prepareForLoop();
	nmb = (numaMemoryBankC *)memoryBanks.getLoopItem(&pos);
	for (; nmb != __KNULL;
		nmb = (numaMemoryBankC *)memoryBanks.getLoopItem(&pos))
	{
		if (nmb == getBank(CHIPSET_MEMORY_NUMA___KSPACE_BANKID)) {
			continue;
		};

		nSet = nmb->merge(
			getBank(CHIPSET_MEMORY_NUMA___KSPACE_BANKID));
	};

	__kprintf(NOTICE MEMTRIB"%d frames were merged from __kspace into new "
		"PMM state.\n", nSet);

	// Then apply the Memory Tributary's Memory Regions to all banks.
	if (chipsetRegionMap != __KNULL)
	{
		pos = memoryBanks.prepareForLoop();
		nmb = (numaMemoryBankC *)memoryBanks.getLoopItem(&pos);
		for (; nmb != __KNULL;
			nmb = (numaMemoryBankC *)memoryBanks.getLoopItem(&pos))
		{
			for (uarch_t i=0; i<chipsetRegionMap->nEntries; i++)
			{
				nmb->mapMemUsed(
					chipsetRegionMap->entries[i].baseAddr,
					PAGING_BYTES_TO_PAGES(
						chipsetRegionMap
							->entries[i].size));
			};
		};
	};

	// Allocate BMPs for default config and orientation config.
	pos = prevPos = memoryBanks.prepareForLoop();
	for (; pos != HWIDLIST_INDEX_INVALID; memoryBanks.getLoopItem(&pos)) {
		prevPos = pos;
	};

	ret = defaultAffinity.memBanks.initialize(prevPos);
	if (ret != ERROR_SUCCESS)
	{
		__kprintf(ERROR MEMTRIB"Unable to alloc bmp for defAffin.\n");
		return ERROR_MEMORY_NOMEM;
	};

	ret = cpuTrib.getCurrentCpuStream()->currentTask
		->localAffinity.memBanks.initialize(prevPos);

	if (ret != ERROR_SUCCESS)
	{
		__kprintf(ERROR MEMTRIB"Unable to alloc bmp for orientation "
			"config.\n");

		return ERROR_MEMORY_NOMEM;
	};

	pos = prevPos = memoryBanks.prepareForLoop();
	for (; pos != HWIDLIST_INDEX_INVALID; memoryBanks.getLoopItem(&pos))
	{
		defaultAffinity.memBanks.setSingle(pos);
		cpuTrib.getCurrentCpuStream()->currentTask
			->localAffinity.memBanks.setSingle(pos);
	};

	// And *finally*, see whether or not to destroy __kspace.
	if (__kspaceBool == 1)
	{
		/* To destroy __kspace, we must stop the Orientation thread's
		 * config from pointing to it. We also have to patch up the
		 * NUMA Tributary's default config to stop pointing to it as
		 * well.
		 *
		 * Next, if there is a shared bank, we point the both NUMA
		 * Trib and the orientation thread to it, and move on.
		 *
		 * If there is no shared bank, we should be able to leave it
		 * alone, and the next time a thread asks for pmem, the code in
		 * numaTrib.cpp should auto-determine which bank to set as the
		 * new default.
		 **/
		memoryBanks.removeItem(CHIPSET_MEMORY_NUMA___KSPACE_BANKID);
		__kprintf(NOTICE MEMTRIB"Removed __kspace. Ret is 0x%P.\n",
			getBank(CHIPSET_MEMORY_NUMA___KSPACE_BANKID));

#ifdef CHIPSET_MEMORY_NUMA_GENERATE_SHBANK
		defaultAffinity.def.rsrc = CHIPSET_MEMORY_NUMA_SHBANKID;
		cpuTrib.getCurrentCpuStream()->currentTask
			->localAffinity.def.rsrc =
			CHIPSET_MEMORY_NUMA_SHBANKID;

		__kprintf(NOTICE MEMTRIB"Orientation and default "
			"config patched to use shbank as default.\n");
#endif
	};

	return ERROR_SUCCESS;
}

#if __SCALING__ >= SCALING_CC_NUMA
void memoryTribC::init2_spawnNumaStreams(chipsetNumaMapS *map)
{
	error_t		ret;

	for (uarch_t i=0; i<map->nMemEntries; i++)
	{
		if (getBank(map->memEntries[i].bankId) != __KNULL) {
			continue;
		};

		ret = createBank(map->memEntries[i].bankId);
		if (ret != ERROR_SUCCESS)
		{
			__kprintf(ERROR MEMTRIB"Failed to spawn memory bank "
				"for %d.\n",
				map->memEntries[i].bankId);

			continue;
		};

		__kprintf(NOTICE MEMTRIB"Spawned NUMA Stream for bank with ID "
			"%d.\n", map->memEntries[i].bankId);
	};
}
#endif

#if __SCALING__ >= SCALING_CC_NUMA
void memoryTribC::init2_generateNumaMemoryRanges(
	chipsetNumaMapS *map, sarch_t *__kspaceBool
	)
{
	error_t			ret;
	numaMemoryBankC		*nmb;

	for (uarch_t i=0; i<map->nMemEntries; i++)
	{
		nmb = getBank(map->memEntries[i].bankId);
		if (nmb == __KNULL)
		{
			__kprintf(ERROR MEMTRIB"Bank %d found in NUMA map, "
				"but no bank obj in mem trib list.\n",
				map->memEntries[i].bankId);

			continue;
		};

		ret = nmb->addMemoryRange(
			map->memEntries[i].baseAddr,
			map->memEntries[i].size);

		if (ret != ERROR_SUCCESS)
		{
			__kprintf(ERROR MEMTRIB"Failed to allocate "
				"memory range obj for range: base 0x%P "
				"size 0x%P on bank %d.\n",
				map->memEntries[i].baseAddr,
				map->memEntries[i].size,
				map->memEntries[i].bankId);
		}
		else
		{
			/* If even one new memory range was generated
			 * successfully, we can destroy __kspace.
			 **/
			*__kspaceBool = 1;
		};
	};
}
#endif

void memoryTribC::init2_generateShbankFromNumaMap(
	chipsetMemConfigS *cfg, chipsetNumaMapS *map, sarch_t *__kspaceBool
	)
{
	error_t		ret;
	paddr_t		tmpBase, tmpSize;

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
			__kprintf(NOTICE MEMTRIB
				"For memrange %d, on bank %d, base 0x%P, size "
				"0x%P, next entry base 0x%P, shbank memory "
				"range with base 0x%P and size 0x%P is needed."
				"\n", i,
				map->memEntries[i].bankId,
				map->memEntries[i].baseAddr,
				map->memEntries[i].size,
				map->memEntries[i+1].baseAddr,
				tmpBase, tmpSize);
#endif

			ret = getBank(CHIPSET_MEMORY_NUMA_SHBANKID)
				->addMemoryRange(tmpBase, tmpSize);

			if (ret != ERROR_SUCCESS)
			{
				__kprintf(ERROR MEMTRIB
					"Shbank: Failed to spawn memrange for "
					"hole: base 0x%P size 0x%P.\n",
					tmpBase, tmpSize);
			}
			else
			{
				__kprintf(NOTICE MEMTRIB
					"Shbank: New memrange base 0x%P, size "
					"0x%P.\n",
					tmpBase, tmpSize);

				*__kspaceBool = 1;
			};
		};
	};
}

