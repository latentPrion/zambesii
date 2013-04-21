
#include <chipset/zkcm/numaMap.h>
#include <chipset/zkcm/smpMap.h>
#include <__kstdlib/__ktypes.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/processTrib/processTrib.h>

#if __SCALING__ >= SCALING_SMP
cpuStreamC *cpuTribC::getCurrentCpuStream(void)
{
	uarch_t		dr0;
	// Read the current cpu's struct from the dr0 register.
	asm volatile (
		"movl %%dr0, %0 \n\t"
		: "=r" (dr0)
	);

	return reinterpret_cast<cpuStreamC *>( dr0 );
}
#endif

