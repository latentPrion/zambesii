#ifndef _CHIPSET_NUMA_MAP_H
	#define _CHIPSET_NUMA_MAP_H

	#include <arch/paddr_t.h>
	#include <__kstdlib/__ktypes.h>

/**	EXPLANATION:
 * Another interface for compile-time kernel configuration. Use this to compile
 * in NUMA configuration for the kernel to use.
 **/

#define NUMAMEMMAP_FLAGS_HOTPLUG	(1<<0)
#define NUMAMEMMAP_FLAGS_ONLINE		(1<<1)

struct numaMemMapEntryS
{
	paddr_t		baseAddr, size;
	sarch_t		bankId;
	uarch_t		flags;
};

struct chipsetNumaMapS
{
	// numaCpuMapEntryS	*cpuEntries;
	struct numaMemMapEntryS	*memEntries;
	ubit8			nMemEntries, nCpuEntries;
};

#endif

