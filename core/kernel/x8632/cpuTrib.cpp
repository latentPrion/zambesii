
#include <__kstdlib/__ktypes.h>
#include <__kthreads/__korientation.h>
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
#endif

error_t cpuTribC::initialize(void)
{
	uarch_t		dr0;

	// Spawn the BSP CPU Stream.
	bspCpu.id = CPUID_INVALID;
	bspCpu.bankId = NUMABANKID_INVALID;
	bspCpu.currentTask = &__korientationThread;
	bspCpu.cpuFeatures.fpuLevel = 0;
	bspCpu.cpuFeatures.mhz = 0;

	/**	EXPLANATION:
	 * This one line is VERY important. If you don't initialize the BSP
	 * CPU Stream with this magic number, the global object constructor call
	 * sequence that will run on it later will overwrite the arch-specific
	 * values with normal constructed values. This will cause the BSP CPU's
	 * Stream to be overwritten, and the pointer to the current task will
	 * be corrupted.
	 *
	 * Every arch MUST make sure to set this magic number in the BSP CPU
	 * init before exiting. There will be a clean up on this later on after
	 * the CPU Tributary is written up.
	 *
	 * In other words, the purpose of this magic number is to protect the
	 * BSP CPU Stream instance from being overwritten at boot during the
	 * global constructor call.
	 **/
	bspCpu.initMagic = CPUSTREAM_INIT_MAGIC;

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

