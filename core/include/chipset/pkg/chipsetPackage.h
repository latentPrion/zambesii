#ifndef _CHIPSET_PACKAGE_H
	#define _CHIPSET_PACKAGE_H

	#include <__kstdlib/__ktypes.h>
	#include "watchdogMod.h"
	#include "memoryMod.h"
	#include "cpuMod.h"
	#include "debugMod.h"

struct chipsetPackageS
{
	error_t (*initialize)(void);
	error_t (*shutdown)(void);
	error_t (*suspend)(void);
	error_t (*restore)(void);

	struct watchdogPgkS	*watchdog;
	struct memoryModS	*memory;
	struct cpuModS		*cpus;

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

