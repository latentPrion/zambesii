#ifndef _CHIPSET_PACKAGE_H
	#define _CHIPSET_PACKAGE_H

	#include <__kstdlib/__ktypes.h>
	#include "watchdogMod.h"
	#include "memoryMod.h"
	#include "cpuMod.h"
	#include "debugMod.h"
	#include "intControllerMod.h"

struct chipsetPackageS
{
	error_t (*initialize)(void);
	error_t (*shutdown)(void);
	error_t (*suspend)(void);
	error_t (*restore)(void);

	// Controls the chipset's watchdog device.
	struct watchdogModS		*watchdog;
	// Enumerates memory ranges along with NUMA locality if it applies.
	struct memoryModS		*memory;
	// Enumerates CPUs, and any NUMA binding associated with them.
	struct cpuModS			*cpus;
	// Manages IRQs on the chipset.
	struct intControllerModS	*intController;

	struct debugModS	*debug[4];
};

#ifdef __cplusplus
#define PKGEXTERN	extern "C"
#else
#define PKGEXTERN	extern
#endif

extern struct chipsetPackageS		chipsetPkg;

#undef PKGEXTERN

#endif

