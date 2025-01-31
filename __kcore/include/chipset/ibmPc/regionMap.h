#ifndef _CHIPSET_MEMORY_REGION_MAP_H
	#define _CHIPSET_MEMORY_REGION_MAP_H
/**	EXPLANATION:
 * Cater for the x86-32 PAE enabled kernel's need to allocate PDP tables from
 * within the first 4GB. We reserve a 512KB region for the kernel in that
 * case.
 *
 * This region will be used to allocate PDP tables. That gives us 256 frames to
 * use up before we have no room left for page directory pointers. In other
 * words, on x86-32 with PAE, we limit the kernel effectively to 256 allocatable
 * address spaces, and by extension, 256 processes max. Amusing.
 **/
enum chipsetMemoryRegionsE
{
	CHIPSET_MEMORY_REGION_ISA_DMA = 0,
#ifdef CONFIG_ARCH_x86_32_PAE
	CHIPSET_MEMORY_REGION_x86_32_PAE_PDP,
#endif
	CHIPSET_MEMORY_NREGIONS
};

#endif
