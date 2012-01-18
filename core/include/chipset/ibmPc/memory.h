#ifndef _CHIPSET_MEMORY_H
	#define _CHIPSET_MEMORY_H

	#include <scaling.h>
	#include <__kstdlib/__ktypes.h>

// __kspace size is 2MB, starting at the 4MB mark.
#define CHIPSET_MEMORY___KSPACE_BASE				0x400000
#define CHIPSET_MEMORY___KSPACE_SIZE				(0x100000 * 3)

// Implies the size of the global array of process pointers.
#define CHIPSET_MEMORY_MAX_NPROCESSES		(16384)
// The maximum number of threads per process.
#define CHIPSET_MEMORY_MAX_NTASKS		(256)


#ifdef CONFIG_ARCH_x86_32
// x86-32 stack size for PC is 2 pages.
#define CHIPSET_MEMORY___KSTACK_NPAGES				2
#endif


#ifndef __ASM__
	#include <__kclasses/hardwareIdList.h>

// The array of reserved memory for __kspace.
extern uarch_t					__kspaceInitMem[];
extern hardwareIdListC::arrayNodeS		initialMemoryBankArray[];
extern hardwareIdListC::arrayNodeS		initialCpuBankArray[];
extern hardwareIdListC::arrayNodeS		initialNumaStreamArray[];
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

#if __SCALING__ >= SCALING_CC_NUMA
#define CHIPSET_MEMORY_NUMA_GENERATE_SHBANK
#define CHIPSET_MEMORY_NUMA_SHBANKID			31
#endif

#endif

