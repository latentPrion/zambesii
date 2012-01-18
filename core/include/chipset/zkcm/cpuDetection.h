#ifndef _ZKCM_CPU_DETECTION_MOD_H
	#define _ZKCM_CPU_DETECTION_MOD_H

	#include <chipset/zkcm/smpMap.h>
	#include <chipset/zkcm/numaMap.h>
	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/smpTypes.h>

struct zkcmCpuDetectionModS
{
	error_t (*initialize)(void);
	error_t (*shutdown)(void);
	error_t (*suspend)(void);
	error_t (*restore)(void);

	sarch_t (*checkSmpSanity)(void);
	cpu_t (*getBspId)(void);
	struct zkcmSmpMapS *(*getSmpMap)(void);
	struct zkcmNumaMapS *(*getNumaMap)(void);
	status_t (*powerControl)(cpu_t cpuId, ubit8 command, uarch_t flags);
};

#endif

