#include <chipset/dedicatedMemoryRegions.h>
#include <chipset/ibmPc/dedicatedMemoryRegions.h>
#include <__kstdlib/__ktypes.h>


static sDedicatedMemoryRegionMapEntry	dedicatedRegions[] =
{
	{
		// DMA region. We reserve 2MB.
		0x1000, 0x80000,
		NULL,
		0
	}
#ifdef CONFIG_ARCH_x86_32_PAE
	/**	EXPLANATION:
	 * Cater for the x86-32 PAE enabled kernel's need to allocate PDP
	 * tables from within the first 4GB. We reserve a 2MiB region for
	 * the kernel in that case.
	 *
	 * This region will be used to allocate PDP tables. That gives us
	 * 512 frames to use up before we have no room left for page
	 * directory pointers. In other words, on x86-32 with PAE, we
	 * limit the kernel effectively to 512 allocatable address
	 * spaces, and by extension, 512 processes max. Amusing.
	 **/
	,{
		// PAE x86-32 PDP table allocator region. 2MiB.
		(CHIPSET_MEMORY___KSPACE_BASE + CHIPSET_MEMORY___KSPACE_SIZE),
		0x200000,
		NULL,
		0
	}
#endif
};

static sDedicatedMemoryRegionMap	_dedicatedRegionMap =
{
	dedicatedRegions,
	CHIPSET_DEDICATED_MEMRGN_NREGIONS
};

sDedicatedMemoryRegionMap		*dedicatedMemoryRegionMap =
	&_dedicatedRegionMap;
