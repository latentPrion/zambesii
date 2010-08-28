#ifndef _CHIPSET_NUMA_MAP_H
	#define _CHIPSET_NUMA_MAP_H

	#include <arch/paddr_t.h>
	#include <__kstdlib/__ktypes.h>

/**	EXPLANATION:
 * Another interface for compile-time kernel configuration. Use this to compile
 * in NUMA configuration for the kernel to use.
 **/

struct numaMemMapEntryS
{
	paddr_t		baseAddr, size;
};

struct chipsetNumaMapS
{
	// numaCpuMapEntryS	*cpuConfig;
	numaMemMapEntryS	*memConfig;
	ubit8			nMemMapEntries, nCpuMapEntries;
};

#endif

