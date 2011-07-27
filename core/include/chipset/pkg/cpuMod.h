#ifndef _CPU_INFO_MODULE_H
	#define _CPU_INFO_MODULE_H

	#include <chipset/numaMap.h>
	#include <chipset/smpMap.h>
	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/smpTypes.h>

struct cpuModS
{
	error_t (*initialize)(void);
	error_t (*shutdown)(void);
	error_t (*suspend)(void);
	error_t (*restore)(void);

	struct chipsetNumaMapS *(*getNumaMap)(void);
	struct chipsetSmpMapS *(*getSmpMap)(void);
	/* This MUST be called at least once BEFORE CPU detection begins, and
	 * must be called ON THE BSP.
	 **/
	cpu_t (*getBspId)(void);
};

#endif

