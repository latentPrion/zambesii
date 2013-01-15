
#include <__kstdlib/__ktypes.h>
#include <kernel/common/cpuTrib/cpuTrib.h>

#if __SCALING__ >= SCALING_SMP
cpuStreamC *cpuTribC::getCurrentCpuStream(void)
{
	uarch_t		dr0;
	// Read the current cpu's struct from the fs and gs registers.
	asm volatile (
		"movl %%dr0, %0 \n\t"
		: "=r" (dr0)
	);
	return reinterpret_cast<cpuStreamC *>( dr0 );
}
#endif

