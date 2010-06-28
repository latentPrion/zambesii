
#include <__kstdlib/__ktypes.h>
#include <__kthreads/__korientationPreConstruct.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/cpuTrib/cpuStream.h>

#if __SCALING__ >= SCALING_SMP
cpuStreamC *cpuTribC::getCurrentCpuStream(void)
{
	uarch_t		dr0;
	// Read the current cpu's struct from the fs and gs registers.
	asm volatile (
		"movl %%dr0, %0\n\t"
		: "=r" (dr0)
	);
	return reinterpret_cast<cpuStreamC *>( dr0 );
}

cpuStreamC *cpuTribC::getStream(cpu_t cpu)
{
	/* This should be okay for now. We can reshuffle the pointers when we
	 * have the hardware ids of the CPUs.
	 **/
	return &(*cpuStreams.rsrc)[cpu];
}
#endif

void cpuTribC::preConstruct(void)
{
	uarch_t		dr0;

	// Spawn the BSP CPU Stream.
	bspCpu.id = bspCpu.bankId = 0;
	bspCpu.currentTask = &__korientationThread;
	bspCpu.cpuFeatures.fpuLevel = 0;
	bspCpu.cpuFeatures.mhz = 0;

	dr0 = reinterpret_cast<uarch_t>( &bspCpu );

	asm volatile (
		"pushl %%eax \n\t \
		movl %%dr7, %%eax \n\t \
		andl $0xFFFFFE00, %%eax \n\t \
		movl %%eax, %%dr7 \n\t \
		popl %%eax \n\t \
		\
		movl %0, %%dr0 \n\t"
		:
		: "r" (dr0)
		: "%eax"
	);
}

