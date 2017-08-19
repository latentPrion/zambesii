
#include <arch/cpuControl.h>
#include <arch/debug.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include "exceptions.h"


status_t __attribute__((noreturn)) x8632_breakpoint(RegisterContext *regs, ubit8)
{
	Thread		*currThread=NULL;

	currThread = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();
	printf(NOTICE"Breakpoint exception on CPU %d.\n"
		"\tContext: CS %x, EIP %x, EFLAGS %x\n"
		"\tESP %p, EBP %p, stack0 %p, stack1 %p\n"
			"\tESI %x, EDI %x\n"
		"\tEAX %x, EBX %x, ECX %x, EDX %x.\n",
		cpuTrib.getCurrentCpuStream()->cpuId,
		regs->cs, regs->eip, regs->eflags,
		regs->esp, regs->ebp,
		currThread->stack0, currThread->stack1,
		regs->esi, regs->edi,
		regs->eax, regs->ebx, regs->ecx, regs->edx);

	debug::sStackDescriptor		currStackDesc;

	debug::getCurrentStackInfo(&currStackDesc);
	if (currThread != NULL)
		{ printf(NOTICE"This is a normal thread.\n"); }
	else
		{ printf(NOTICE"This is a per-cpu thread.\n"); };

	debug::printStackTrace(
		(void *)regs->ebp, &currStackDesc);

	for (;FOREVER;)
	{
		cpuControl::disableInterrupts();
		cpuControl::halt();
	};
}

