
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
	 *
	 * Waking CPUs MUST NOT allocate memory from the kernel address space
	 * or do anything which would require accessing or changing the kernel
	 * page tables and translation information. Right now, other CPUs are
	 * waking up, and when a kernel page is mapped anew, we need to
	 * flush the TLB on all other CPUs.
	 *
	 * But since we're currently waking APs, we could send out a global
	 * TLB flush request before a CPU is fully initialized and in a state
	 * to handle inter-CPU-messages, and thus cause that waking CPU to
	 * crash.
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

	// Halt the CPU here.
	for (;;) {
		asm volatile ("hlt \n\t");
	};
}

