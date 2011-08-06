#ifndef _x86_32_CPUID_H
	#define _x86_32_CPUID_H

	#include <__kstdlib/__ktypes.h>

extern "C" void execCpuid(
	ubit8 num, uarch_t *eax, uarch_t *ebx, uarch_t *ecx, uarch_t *edx);

#endif

