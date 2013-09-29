
#include <arch/cpuControl.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include "exceptions.h"


status_t __attribute__((noreturn)) x8632_breakpoint(registerContextC *regs, ubit8)
{
	taskC		*currTask;

	currTask = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask();
	__kprintf(NOTICE"Debug exception on CPU %d (%s thread).\n"
		"\tContext: CS 0x%x, EIP 0x%x, EFLAGS 0x%x\n"
		"\tESP 0x%p, EBP 0x%p, stack0 0x%p, stack1 0x%p\n"
		"\tESI 0x%x, EDI 0x%x\n"
		"\tEAX 0x%x, EBX 0x%x, ECX 0x%x, EDX 0x%x.\n",
		cpuTrib.getCurrentCpuStream()->cpuId,
		(currTask->getType() == task::PER_CPU) ? "per-cpu" : "unique",
		regs->cs, regs->eip, regs->eflags,
		regs->esp, regs->ebp,
		(currTask->getType() == task::PER_CPU)
			? cpuTrib.getCurrentCpuStream()->perCpuThreadStack
			: ((threadC *)currTask)->stack0,
		(currTask->getType() == task::UNIQUE)
			? ((threadC *)currTask)->stack1 : NULL,
		regs->esi, regs->edi,
		regs->eax, regs->ebx, regs->ecx, regs->edx);

	for (;;)
	{
		cpuControl::disableInterrupts();
		cpuControl::halt();
	};
}

