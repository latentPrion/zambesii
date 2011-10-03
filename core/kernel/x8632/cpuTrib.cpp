
#include <arch/io.h>
#include <chipset/chipset.h>
#include <chipset/memoryAreas.h>
#include <__kstdlib/__ktypes.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kthreads/__korientation.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/cpuTrib/cpuStream.h>

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

error_t cpuTribC::initialize(void)
{
	uarch_t		dr0;

	// Spawn the BSP CPU Stream.
	bspCpu.id = CPUID_INVALID;
	bspCpu.bankId = NUMABANKID_INVALID;
	bspCpu.taskStream.currentTask = &__korientationThread;
	bspCpu.cpuFeatures.fpuLevel = 0;
	bspCpu.cpuFeatures.clockMhz = 0;
	// Let the CPU know that it is the BSP.
	__KFLAG_SET(bspCpu.flags, CPUSTREAM_FLAGS_BSP);

	// Load the BSP's DR0 with the addr of the BSP's stream.
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

	return ERROR_SUCCESS;
}

