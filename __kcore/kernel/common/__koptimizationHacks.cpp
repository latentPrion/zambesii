
#include <arch/arch.h>
#include <kernel/common/__koptimizationHacks.h>

void __koptimizationHacks(void)
{
	__kheadersInit();
#ifdef __x86_32__
	__kcpuPowerOnInit();
#endif
}

