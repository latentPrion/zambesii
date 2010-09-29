#ifndef _CHIPSET_CPU_MAP_H
	#define _CHIPSET_CPU_MAP_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/smpTypes.h>


/**	EXPLANATION:
 * Every chipset which has SMP or NUMA physical configuration SHALL provide a
 * CPU map. This map MUST contain an entry for every physical CPU present at
 * boot, regardless of whether or not it's a "bad" CPU (actually, especially if
 * it's a bad CPU), or if it's offline at boot.
 *
 * For CPU hotplug, there need not be an entry for an emplty slot which will
 * have a CPU plugged in at runtime. But as long as a CPU is expected to
 * plugged in when the kernel is booting, this CPU must be in the CPU map.
 *
 * In other words, this is a fully detailed map returned by the chipset code
 * which will tell the kernel how to handle any CPU at boot.
 *
 * At boot, the kernel will generate CPU Streams for all CPUs whose entries
 * mark them as "good" (i.e., not BADCPU) and "online".
 *
 * At runtime, if a new CPU is detected by some driver of sorts, the kernel will
 * use the information returned from the API that enumerated the hotplug CPU
 * to decide how to handle it.
 **/

#define CHIPSETCPUMAP_FLAGS_BADCPU		(1<<0)

struct chipsetCpuMapEntryS
{
	cpu_t		cpuId;
	uarch_t		flags;
};

struct chipsetCpuMapS
{
	uarch_t			nEntries;
	chipsetCpuMapEntryS	*entries;
};

#endif

