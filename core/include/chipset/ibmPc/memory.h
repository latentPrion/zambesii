#ifndef _CHIPSET_MEMORY_H
	#define _CHIPSET_MEMORY_H

// Absolute load address of the kernel in physical memory.
#define CHIPSET_MEMORY___KLOAD_PADDR_BASE		(0x100000)
#define CHIPSET_MEMORY___KBOG_SIZE			(0x200000)

#define CHIPSET_MEMORY_NUMA_GENERATE_SHBANK
#define CHIPSET_MEMORY_NUMA_SHBANK_ID			(16)

#endif

