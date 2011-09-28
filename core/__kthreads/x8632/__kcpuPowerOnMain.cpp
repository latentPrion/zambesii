
#include <debug.h>
#include <arch/paging.h>
#include <__kstdlib/__ktypes.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <__kthreads/__kcpuPowerOn.h>


#define CPUPOWER		"CPU Poweron: "

void __kcpuPowerOnMain(void)
{
	cpuStreamC	*myStream;

	/**	EXPLANATION:
	 * Main path for a waking x86-32 CPU. Do the basics to get the CPU in
	 * synch with the kernel.
	 **/

	// Retrieve the stream pointer and release the lock asap.
	myStream = __kcpuPowerOnBlock.cpuStream;
	myStream->baseInit();
	__kcpuPowerOnBlock.lock.release();

	// Print a message to allow for CPU wakeup tracing.
	__kprintf(NOTICE CPUPOWER"CPU %d: Entered CPU power on thread.\n",
		myStream->cpuId);

	// Enumerates, sets up, etc.
	myStream->initialize();
	asm volatile ("hlt \n\t");
}

