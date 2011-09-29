#ifndef _CPU_FEATURES_H
	#define _CPU_FEATURES_H

	#include <__kstdlib/__ktypes.h>
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
	utf8Char		/**cpuFamily, */*cpuName;
	ubit16			clockMhz;
	ubit8			fpuLevel;
	ubit8			l1CacheSize;
	ubit8			cacheLineSize;
	ubit8			bitWidths;
	// Arch specific CPU features.
	archCpuFeaturesS	archFeatures;
};

#endif

