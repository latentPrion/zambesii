#ifndef _CPU_INFO_RIV_H
	#define _CPU_INFO_RIV_H

	#include <chipset/numaMap.h>
	#include <__kstdlib/__ktypes.h>

/**	EXPLANATION:
 * Every chipset that does SMP or NUMA shall provide a cpuInfoRiv. This rivulet
 * will return, for a chipset that has a NUMA set-up, a NUMA map, which need not
 * be the same as the one returned from memInfoRivS.getNumaMap().
 *
 * Any chipset which does SMP (this is for NUMA chipsets as well) must provide
 * a CPU Map, which is a much like a memory map, that tell the kernel about each
 * CPU on the chipset, regardless of NUMA affinity. This map must tell of ALL
 * CPUs on the chipset, *INCLUDING* those which should not be booted by the
 * kernel.
 *
 * There are flags in each entry of the CPU Map. If a CPU is not to be booted,
 * the CHIPSETCPUMAP_FLAGS_BADCPU flag should be set for that CPU entry. DO
 * *NOT* leave bad CPUs out of the map.
 **/

struct cpuInfoRivS
{
	struct chipsetNumaMapS *(*getNumaMap)(void);
	struct chipsetCpuMapS *(*getCpuMap)(void);
};

#endif

