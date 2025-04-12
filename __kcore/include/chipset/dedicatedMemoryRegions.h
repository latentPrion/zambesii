#ifndef _CHIPSET_DEDICATED_MEMORY_REGIONS_H
	#define _CHIPSET_DEDICATED_MEMORY_REGIONS_H

	#include <chipset/chipset_include.h>
	#include <arch/paddr_t.h>
	#include <__kstdlib/__ktypes.h>

/**	EXPLANATION:
 * sDedicatedMemoryRegionMap is used to detail, at compile time, the dedicated
 * memory ranges existent on the chipset. An example would be the IBM-PC, where
 * there is a 16MB region which most kernels reserve for DMA.
 *
 * Another example is a chipset with 32-bit PCI, which runs a 64-bit cpu. Or
 * even the x86-32 with the PAE enabled. In the latter's case, you'd need a
 * region which is under 4GB from which to allocate PDP tables.
 *
 * Technically this interface won't handle all of those completely gracefully,
 * but at any rate, *much* more cleanly than any other existing kernel to my
 * knowledge.
 *
 * The kernel will scan the entries presented here to determine how to
 * initialize memory regions for the chipset on which it is operating.
 *
 * If the pointer to the memory region map is NULL, then the kernel assumes
 * there are no memory regions.
 **/

struct sDedicatedMemoryReservedRegion
{
	paddr_t		baseAddr, size;
};

struct sDedicatedMemoryRegionMapEntry
{
	paddr_t				baseAddr, size;
	sDedicatedMemoryReservedRegion	*reservedRegions;
	ubit8				nReservedRegions;
};

struct sDedicatedMemoryRegionMap
{
	sDedicatedMemoryRegionMapEntry		*entries;
	ubit8					nEntries;
};

/*	NOTE:
 * Do not remove this. It's used to statically allocate room for the memory
 * regions in memoryTrib.h.
 **/
#include CHIPSET_INCLUDE(dedicatedMemoryRegions.h)

extern sDedicatedMemoryRegionMap		*dedicatedMemoryRegionMap;
void dumpChipsetDedicatedMemoryRegions(void);

#endif /* _CHIPSET_DEDICATED_MEMORY_REGIONS_H */

