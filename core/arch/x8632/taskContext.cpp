
#include <arch/paging.h>
#include <arch/taskContext.h>
#include <arch/x8632/cpuFlags.h>
#include <chipset/memory.h>
#include <kernel/common/process.h>
#include <kernel/common/panic.h>


taskContextC::taskContextC(ubit8 execDomain)
{
	memset(this, 0, sizeof(*this));

	// Set the x86 EFLAGS.IF to enable IRQs when new task is pulled.
	eflags |= x8632_CPUFLAGS_IF;
	if (execDomain == PROCESS_EXECDOMAIN_KERNEL)
	{
		cs = 0x08;
		ds = es = fs = gs = ss = 0x10;
	}
	else
	{
		cs = 0x18;
		ds = es = fs = gs = ss = 0x18;
	};
}

void taskContextC::setStacks(
	void *stack0, void *stack1, ubit8 initialStackIndex
	)
{
	if (initialStackIndex > 1) {
		panic(CC"taskContextC::setStacks(): illegal stack index.\n");
	};

	if (initialStackIndex == 0)
	{
		esp = (ubit32)stack0
			+ (CHIPSET_MEMORY___KSTACK_NPAGES * PAGING_BASE_SIZE);
	}
	else
	{
		esp = (ubit32)stack1
			+ (CHIPSET_MEMORY_USERSTACK_NPAGES * PAGING_BASE_SIZE);
	};
}

