
#include <arch/cpuControl.h>
#include <arch/debug.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include "exceptions.h"


status_t __attribute__((noreturn)) x8632_breakpoint(RegisterContext *regs, ubit8)
{
	Thread		*currThread=NULL;

	currThread = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();
	printf(NOTICE"Breakpoint exception on CPU %d.\n"
		"\tContext: CS 0x%x, EIP 0x%x, EFLAGS 0x%x\n"
		"\tESP 0x%p, EBP 0x%p, stack0 0x%p, stack1 0x%p\n"
			"\tESI 0x%x, EDI 0x%x\n"
		"\tEAX 0x%x, EBX 0x%x, ECX 0x%x, EDX 0x%x.\n",
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

