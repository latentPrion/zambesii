#ifndef _CHIPSET_MEMORY_H
	#define _CHIPSET_MEMORY_H

	#include <scaling.h>
	#include <__kstdlib/__ktypes.h>

// __kspace size is 2MB, starting at the 4MiB mark. Do not exceed 3MiB.
#define CHIPSET_MEMORY___KSPACE_BASE				(0x400000)
#define CHIPSET_MEMORY___KSPACE_SIZE				(0x100000 * 2)

// Implies the size of the global array of process pointers.
#define CHIPSET_MEMORY_MAX_NPROCESSES		(16384)
// The maximum number of threads per process.
#define CHIPSET_MEMORY_MAX_NTASKS		(256)

#ifdef CONFIG_ARCH_x86_32
	// x86-32 stack size for PC is 2 pages.
	#define CHIPSET_MEMORY___KSTACK_NPAGES				2
	#define CHIPSET_MEMORY_USERSTACK_NPAGES				8
#endif

// Absolute load address of the kernel in physical memory.
#define CHIPSET_MEMORY___KLOAD_PADDR_BASE		(0x100000)
#define CHIPSET_MEMORY___KBOG_SIZE			(0x200000)

#endif

