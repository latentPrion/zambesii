#ifndef _CPU_FEATURES_H
	#define _CPU_FEATURES_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kclib/string.h>
	#include <kernel/common/cpuTrib/archCpuFeatures.h>

#define CPUFEAT_FPULEVEL_NONE		0
#define CPUFEAT_FPULEVEL_EXTERNAL	1
#define CPUFEAT_FPULEVEL_INTEGRATED	2

#define CPUFEAT_BITWIDTH_7		(1<<0)
#define CPUFEAT_BITWIDTH_8		(1<<1)
#define CPUFEAT_BITWIDTH_16		(1<<2)
#define CPUFEAT_BITWIDTH_32		(1<<3)
#define CPUFEAT_BITWIDTH_64		(1<<4)

struct cpuFeaturesS
{
	cpuFeaturesS(void)
	{
		memset(this, 0, sizeof(*this));
	}

	utf8Char		cpuFamily[32], cpuModel[64];
	ubit16			clockMhz;
	ubit8			fpuLevel;
	ubit16			l1CacheSize, l2CacheSize, l3CacheSize;
	ubit8			cacheLineSize;
	ubit8			bitWidth;
	// Arch specific CPU features.
	archCpuFeaturesS	archFeatures;
};

#endif

