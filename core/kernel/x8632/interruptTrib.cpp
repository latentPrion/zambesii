
#include <arch/interrupts.h>
#include <arch/x8632/idt.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kclib/string.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/firmwareTrib/firmwareTrib.h>
#include <kernel/common/interruptTrib/interruptTrib.h>
#include <kernel/common/moduleApis/chipsetSupportPackage.h>
#include <kernel/common/moduleApis/interruptController.h>
#include <kernel/common/firmwareTrib/rivIoApi.h>


void interruptTrib_irqEntry(taskContextS *regs)
{
	interruptTrib.irqMain(regs);
	io_write8(0x20, 0x20);

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

void interruptTribC::installHardwareVectorTable(void)
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
			(reinterpret_cast<uarch_t>( __kvectorTable[i] ) >> 16)
			& 0xFFFF;
	};

	/* Hand the now built-up hardware vector table to the hardware. No
	 * need to do any inline ASM stuff since the symbol x8632IdtPtr is
	 * a global symbol.
	 **/
	asm volatile("lidt	(x8632IdtPtr)\n\t");
}

void interruptTribC::installExceptions(void)
{
	for (uarch_t i=0; i<19; i++)
	{
		isrTable[i].handler.except = __kexceptionTable[i];
		__KFLAG_SET(
			isrTable[i].flags,
			INTERRUPTTRIB_VECTOR_FLAGS_EXCEPTION);
	};

	for (uarch_t i=19; i<32; i++)
	{
		isrTable[i].handler.except = __kexceptionTable[19];
		__KFLAG_SET(
			isrTable[i].flags,
			INTERRUPTTRIB_VECTOR_FLAGS_EXCEPTION);
	};
}
