#ifndef _x86_32_CPU_ENUMERATION_H
	#define _x86_32_CPU_ENUMERATION_H

	#include <__kstdlib/__ktypes.h>

#define CPUENUM_UNSUPPORTED_CPU		0x1
#define CPUENUM_CPU_MODEL_UNCLEAR	0x2
#define CPUENUM_CPU_MODEL_UNKNOWN	0x3

namespace x86CpuEnumeration
{
	status_t intel(void);
	status_t amd(void);
}

#endif

