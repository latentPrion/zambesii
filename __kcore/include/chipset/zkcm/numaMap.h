#ifndef _CHIPSET_NUMA_MAP_H
	#define _CHIPSET_NUMA_MAP_H

	#include <arch/paddr_t.h>
	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/numaTypes.h>
	#include <kernel/common/smpTypes.h>

/**	EXPLANATION:
 * The format for a system NUMA map that gives details about the per-bank memory
 * ranges and CPUs in a NUMA setup.
 **/

#define NUMAMEMMAP_FLAGS_HOTPLUG	(1<<0)
#define NUMAMEMMAP_FLAGS_ONLINE		(1<<1)

#define NUMACPUMAP_FLAGS_HOTPLUG	(1<<0)
#define NUMACPUMAP_FLAGS_ONLINE		(1<<1)

struct sNumaCpuMapEntry
{
	numaBankId_t	bankId;
	cpu_t		cpuId;
	ubit32		cpuAcpiId;
	uarch_t		flags;
};

struct sNumaMemoryMapEntry
{
	paddr_t		baseAddr, size;
	numaBankId_t	bankId;
	uarch_t		flags;
};

struct sZkcmNumaMap
{
	struct sNumaCpuMapEntry	*cpuEntries;
	struct sNumaMemoryMapEntry	*memEntries;
	ubit8			nMemEntries, nCpuEntries;
};

#endif

