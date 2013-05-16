#include <chipset/regionMap.h>
#include <chipset/ibmPc/regionMap.h>
#include <__kstdlib/__ktypes.h>


static chipsetRegionMapEntryS	chipsetRegions[] =
{
	{
		// DMA region. We reserve 2MB.
		0x1000, 0x80000,
		__KNULL,
		0
	}
#ifdef CONFIG_ARCH_x86_32_PAE
	,{
		// PAE x86-32 PDP table allocator region. 1MB.
		0x700000, 0x100000,
		__KNULL,
		0
	}
#endif
};

static chipsetRegionMapS	_chipsetRegionMap =
{
	chipsetRegions,
	CHIPSET_MEMORY_NREGIONS
};

chipsetRegionMapS		*chipsetRegionMap = &_chipsetRegionMap;

