
#include <arch/x8632/gdt.h>
#include <__kstdlib/__ktypes.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


void __kcpuPowerOnHll(void)
{
	void		*stackAddr;

	/**	EXPLANATION:
	 * This function initially has one aim; when it accomplishes that it
	 * moves on to whatever else is needed. That initial aim is to switch
	 * away from the temporary stack it's currently using and release the
	 * lock found in __kcpuPowerOnEntry.S.
	 **/
	stackAddr = static_cast<void *>( &__kcpuPowerOnSelf->sleepStack[48] );

	asm volatile (
		"movl	%0, %%esp \n\t"
		:
		: "r" (stackAddr));

	// Stack should be switched now. Let compiler set up new stack frame.
	__kcpuPowerOnMain();
}

void __kcpuPowerOnMain(void)
{
	cpuStreamC	*myStream = __kcpuPowerOnSelf;

	// Release lock:
	asm volatile (
		"movl	$0, %eax \n\t \
		xchg	%eax, (__kcpuPowerOnStackLock) \n\t");

	// Load kernel's main GDT:
	asm volatile ("lgdt	(x8632GdtPtr)");
	// Load the kernel's IDT:
	asm volatile ("lidt	(x8632IdtPtr)");

	__kprintf(NOTICE"Power on thread: CPU %d powered on.\n",
		myStream->cpuId);

	asm volatile ("hlt\n\t");
}

