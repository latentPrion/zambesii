#ifndef _ARCH_x86_32_CPU_FEATURES_H
	#define _ARCH_x86_32_CPU_FEATURES_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kclib/string.h>

#define x8632_CPUFEAT_BASE_SYSENTER	(1<<0)

#define x8632_CPUFEAT_ADVANCED_VM	(1<<0)

#define x8632_CPUFEAT_SSE_NONE		0
#define x8632_CPUFEAT_SSE1		1
#define x8632_CPUFEAT_SSE2		2
#define x8632_CPUFEAT_SSE3		3

struct archCpuFeaturesS
{
	archCpuFeaturesS(void)
	{
		memset(this, 0, sizeof(*this));
	}

	utf8Char	manufacturer[32];
	ubit32		baseFlags;
	ubit32		advancedFlags;
	ubit8		sseLevel;
	ubit8		cpuNameNSpaces;

	struct
	{
		ubit8	f00fBug;
	} errataInfo;

	struct
	{
		ubit8	family, stepping, model, revision;
	} modelInfo;
};

#endif

