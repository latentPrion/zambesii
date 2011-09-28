#ifndef _x86_32_CPUID_H
	#define _x86_32_CPUID_H

	#include <__kstdlib/__ktypes.h>

extern "C" void execCpuid(
	uarch_t num, uarch_t *eax, uarch_t *ebx, uarch_t *ecx, uarch_t *edx);

extern "C" sarch_t x8632_verifyCpuIsI486OrHigher(void);

#endif

