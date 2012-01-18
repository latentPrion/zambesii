#ifndef _ZK_CHIPSET_MODULE_CORE_H
	#define _ZK_CHIPSET_MODULE_CORE_H

	#include <__kstdlib/__ktypes.h>
	#include <chipset/zkcm/memoryDetection.h>
	#include <chipset/zkcm/cpuDetection.h>
	#include <chipset/pkg/watchdogMod.h>
	#include <chipset/pkg/intControllerMod.h>
	#include <chipset/pkg/debugMod.h>

struct zkcmCoreS
{
	utf8Char	chipsetName[96];
	utf8Char	chipsetVendor[96];

	error_t (*initialize)(void);
	error_t (*shutdown)(void);
	error_t (*suspend)(void);
	error_t (*restore)(void);

	struct zkcmMemoryDetectionModS		*memoryDetection;
	struct zkcmCpuDetectionModS		*cpuDetection;

	// XXX: Still to be updated.
	struct watchdogModS			*watchdog;
	struct intControllerModS		*intController;

	struct debugModS			*debug[4];
};

extern struct zkcmCoreS		zkcmCore;

#endif

