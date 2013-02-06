
#include <arch/paging.h>
#include <arch/taskContext.h>
#include <chipset/memory.h>
#include <kernel/common/process.h>


taskContextC::taskContextC(ubit8 execDomain)
{
	memset(this, 0, sizeof(*this));
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

void taskContextC::setStacks(ubit8 execDomain, void *stack0, void *stack1)
{
	if (execDomain == PROCESS_EXECDOMAIN_KERNEL)
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

