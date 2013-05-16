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

#if __SCALING__ >= SCALING_CC_NUMA
	// For a NUMA build, set __kspace bank to a nice high number.
	#define CHIPSET_MEMORY_NUMA___KSPACE_BANKID	32
#else
	// On non-NUMA there will only be __kspace & shbank. Set __kspace to 1.
	#define CHIPSET_MEMORY_NUMA___KSPACE_BANKID	1
#endif

/* Shared bank is always generated for IBM-PC. If you are building a custom
 * kernel for a NUMA supporting PC compatible, and you do not want a shared bank
 * you can comment this out though.
 *
 * Do not comment this out on a non-NUMA build. Shared bank must be generated on
 * non-NUMA builds.
 **/
#define CHIPSET_MEMORY_NUMA_GENERATE_SHBANK

#if __SCALING__ >= SCALING_CC_NUMA
	#define CHIPSET_MEMORY_NUMA_SHBANKID			31
#else
	#define CHIPSET_MEMORY_NUMA_SHBANKID			0
#endif

#endif

