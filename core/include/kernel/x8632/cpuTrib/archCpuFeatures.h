#ifndef _ARCH_x86_32_CPU_FEATURES_H
	#define _ARCH_x86_32_CPU_FEATURES_H

	#include <__kstdlib/__ktypes.h>

#define x8632_CPUFEAT_BASE_SYSENTER	(1<<0)

#define x8632_CPUFEAT_ADVANCED_VM	(1<<0)

#define x8632_CPUFEAT_SSE_NONE		0
#define x8632_CPUFEAT_SSE1		1
#define x8632_CPUFEAT_SSE2		2
#define x8632_CPUFEAT_SSE3		3

struct archCpuFeaturesS
{
	ubit32		base;
	ubit32		advanced;
	ubit8		sseLevel;
	ubit8		cpuNameNSpaces;
};

#endif

