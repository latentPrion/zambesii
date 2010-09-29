#ifndef _ARCH_SMP_INFO_H
	#define _ARCH_SMP_INFO_H

	#include <arch/smpMap.h>
	#include <arch/smpConfig.h>
	#include <chipset/numaMap.h>

/**	EXPLANATION:
 * smpInfo:: is a namespace that, for every architecture, will provide
 * a high level API for getting the eventual SMP CPU Map and SMP CPU Config
 * that the kernel will ask the arch code for.
 *
 * It is generally a wrapper around architecture specific CPU detection code
 * for finding CPUs. For example, on x86, this namespace wraps around the
 * MP Specification table parsing code that generates both the CPU Map and the
 * CPU Config, as well as any NUMA CPU Map that the kernel may require in the
 * case of a NUMA setup.
 **/

namespace smpInfo
{
	chipsetNumaMapS *getNumaMap(void);
	archSmpConfigS *getSmpConfig(void);
	archSmpMapS *getSmpMap(void);
}

#endif

