
#include <arch/interrupts.h>
#include <arch/x8632/idt.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kclib/string.h>
#include <kernel/common/firmwareTrib/firmwareTrib.h>
#include <kernel/common/interruptTrib/interruptTrib.h>
#include <kernel/common/moduleApis/chipsetSupportPackage.h>
#include <kernel/common/moduleApis/interruptController.h>

static firmwareStreamS	*stream;

void interruptTrib_irqEntry(taskContextS *regs)
{
	interruptTrib.irqMain(regs);

	// We should be able to: point ESP to regs, and then pop and iret.
	asm volatile(
		"movl	%0, %%esp \n\t \
		popl	%%eax \n\t \
		popl	%%ebx \n\t \
		\
		movw	%%ax, %%es \n\t \
		shr	$16, %%eax \n\t \
		movw	%%ax, %%ds \n\t \
		\
		movw	%%bx, %%gs \n\t \
		shr	$16, %%ebx \n\t \
		movw	%%bx, %%fs \n\t \
		\
		popa \n\t\
		addl	$0x8, %%esp \n\t \
		iret \n\t"
		: "=m" (reinterpret_cast<uarch_t>( regs ))
		);
	// This point should never be reached. Fooey if it is.
}

error_t interruptTribC::initialize1(void)
{
	/* We have a table of vectors. Run through and initialize the IDT with
	 * each pointer.
	 **/
	for (uarch_t i=0; i<ARCH_IRQ_NVECTORS; i++)
	{
		x8632Idt[i].baseLow =
			reinterpret_cast<uarch_t>( __kvectorTable[i] ) & 0xFFFF;

		x8632Idt[i].seg = 0x08;
		x8632Idt[i].rsvd = 0;
		x8632Idt[i].flags = 0x8E;
		x8632Idt[i].baseHigh =
			(reinterpret_cast<uarch_t>( __kvectorTable[i]) >> 16)
			& 0xFFFF;
	};

	/* Hand the now built-up hardware vector table to the hardware. No
	 * need to do any inline ASM stuff since the symbol x8632IdtPtr is
	 * a global symbol.
	 **/
	asm volatile("lidt	(x8632IdtPtr)\n\t");

	/**	EXPLANATION:
	 * At initialization of the Interrupt Tributary, we wish to ensure that
	 * all vectors are masked at the interrupt controller immediately after
	 * loading the hardware vector table into the processor.
	 *
	 * But making a single call to
	 * (*chipsetCoreDev.intController->maskAll)() is not intelligent at all:
	 * we have previously called to the chipset and initialized the
	 * watchdog timer if it exists. In the event that the chipset *does*
	 * have a watchdog, the chipset support code would also have registered
	 * a timer device with the kernel to interrupt at regular intervals and
	 * call the watchdog.
	 *
	 * See, if we call maskAll at the interrupt controller now, we'll mask
	 * off the continuous timer, and hence leave the watchdog unfed. So,
	 * while it would be much faster to just call maskAll, we have to go
	 * one by one, and make sure first that there are no registered ISRs
	 * in the ISR list, and for each unfilled ISR slot, we call
	 * maskSingle() on that slot.
	 *
	 * This is not as bad as it seems for two reasons:
	 *	1. On most processors there aren't a lot of interrupt vectors.
	 *	   x86 is a nice exception in that sense.
	 *	2. Since we loop and mask each interrupt one by one, we get to
	 *	   immediately see, at boot, which interrupts cannot be masked
	 *	   by the chipset support code that was compiled in with our
	 *	   kernel build.
	 **/
	// Check for int controller, halt if none, else call initialize().
	assert_fatal(chipsetCoreDev.intController != __KNULL);
	assert_fatal(
		(*chipsetCoreDev.intController->initialize)() == ERROR_SUCCESS);

	for (uarch_t i=0; i<ARCH_IRQ_NVECTORS; i++)
	{
		if (isrTable[i].flags
}

