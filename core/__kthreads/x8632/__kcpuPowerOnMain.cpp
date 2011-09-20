
#include <debug.h>
#include <arch/paging.h>
#include <arch/x8632/gdt.h>
#include <__kstdlib/__ktypes.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <__kthreads/__kcpuPowerOn.h>


void __kcpuPowerOnHll(void)
{
	cpuStreamC	*myStream;

	/**	EXPLANATION:
	 * This function does the most rudimentary ugly setup of the waking CPU
	 * so that it can execute kernel code. This includes placing a pointer
	 * to its stream into one of the debug regs, and pointing the CPU's
	 * "currentTask" to the CPU power on thread.
	 *
	 * Some of this is likely to be moved into the CPU Stream constructor
	 * code though, when cleanup is done later.
	 **/
	myStream = __kcpuPowerOnBlock.cpuStream;

	// Load DR0 with a pointer to this CPU's CPU Stream.
	asm volatile (
		"pushl	%%eax \n\t \
		movl	%%dr7, %%eax \n\t \
		andl	$0xFFFFFE00, %%eax \n\t \
		movl	%%eax, %%dr7 \n\t \
		popl	%%eax \n\t \
		\
		movl	%0, %%dr0 \n\t"
		:
		: "r" (myStream));

	myStream->currentTask = &__kcpuPowerOnThread;
	__kcpuPowerOnBlock.lock.release();

	__kcpuPowerOnMain(myStream);
}

void __kcpuPowerOnMain(cpuStreamC *self)
{
	/**	EXPLANATION:
	 * Main path for a waking x86-32 CPU. Do the basics to get the CPU in
	 * synch with the kernel.
	 **/

	// Load kernel's main GDT:
	asm volatile ("lgdt	(x8632GdtPtr)");
	// Load the kernel's IDT:
	asm volatile ("lidt	(x8632IdtPtr)");

	__kprintf(NOTICE"Power on thread: CPU %d powered on.\n",
		self->cpuId);

	asm volatile ("hlt\n\t");
}

