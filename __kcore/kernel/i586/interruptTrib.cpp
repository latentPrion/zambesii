
#include <arch/interrupts.h>
#include <arch/x8632/idt.h>
#include <arch/io.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kclib/string.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/interruptTrib/interruptTrib.h>


static void noop(void) {}

void interruptTrib_interruptEntry(RegisterContext *regs)
{
	ubit8		makeNoise;
	CpuStream	*cpuStream;

	cpuStream = cpuTrib.getCurrentCpuStream();

#ifdef CONFIG_DEBUG_INTERRUPTS
	makeNoise = regs->vectorNo != 254 && regs->vectorNo != 32
		&& regs->vectorNo != 34;

	(makeNoise) ? printf(NOTICE OPTS(NOLOG)
		INTTRIB"interruptEntry: CPU %d "
		"entered on vector %d.\n",
		cpuStream->cpuId, regs->vectorNo)
		: noop();
#endif

#ifdef CONFIG_DEBUG_LOCK_EXCEPTIONS
	if (cpuStream->nLocksHeld > 0)
	{
		printf(FATAL INTTRIB"interruptEntry: CPU %d "
			"has %d locks held.\n",
			cpuStream->cpuId, cpuStream->nLocksHeld);
		panic(ERROR_INVALID_STATE);
	};
#endif

	/**	EXPLANATION:
	 * Subtractively decode the type of interrupt vector being dealt with.
	 **/
	if (regs->vectorNo >= ARCH_INTERRUPTS_VECTOR_PIN_START
		&& regs->vectorNo < ARCH_INTERRUPTS_VECTOR_MSI_START)
	{
		interruptTrib.pinIrqMain(regs);
		goto out;
	};

	if (regs->vectorNo >= ARCH_INTERRUPTS_VECTOR_MSI_START
		&& regs->vectorNo < ARCH_INTERRUPTS_VECTOR_MSI_START
			+ ARCH_INTERRUPTS_VECTOR_MSI_MAXNUM)
	{
		//interruptTrib.msiIrqMain(regs);
		goto out;
	}

	if (regs->vectorNo == ARCH_INTERRUPTS_VECTOR_SWI)
	{
		//interruptTrib.swiMain(regs);
		goto out;
	};

	interruptTrib.exceptionMain(regs);

out:
#ifdef CONFIG_DEBUG_INTERRUPTS
	(makeNoise) ? printf(NOTICE OPTS(NOLOG)
		INTTRIB"interruptEntry: Exiting "
		"on CPU %d vector %d.\n",
		cpuStream->cpuId, regs->vectorNo)
		: noop();
#endif

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
		: "=m" (regs)
		);
	// This point should never be reached. Fooey if it is.
}

void InterruptTrib::installVectorRoutines(void)
{
	/* We have a table of vectors. Run through and initialize the IDT with
	 * each pointer.
	 **/
	for (uarch_t i=0; i<ARCH_INTERRUPTS_NVECTORS; i++)
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
}

void InterruptTrib::installExceptionRoutines(void)
{
	// Register all of the exceptions with the Interrupt Trib.
	for (uarch_t i=0; i<19; i++) {
		installException(i, __kexceptionTable[i], 0);
	};

	for (uarch_t i=19; i<32; i++) {
		installException(i, __kexceptionTable[19], 0);
	};
}

