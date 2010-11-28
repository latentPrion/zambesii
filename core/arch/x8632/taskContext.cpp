
#include <arch/paging.h>
#include <chipset/memory.h>
#include <arch/tlbContext.h>
#include <kernel/common/process.h>


error_t taskContext::initialize(taskContextS *tc, taskC *task, void *entryPoint)
{
	if (task->parent->execDomain == PROCESS_EXECDOMAIN_KERNEL)
	{
		tc->cs = 0x08;
		tc->ds = tc->es = tc->fs = tc->gs = tc->ss = 0x10;
	}
	else
	{
		tc->cs = 0x18;
		tc->ds = tc->es = tc->fs = tc->gs = tc->ss = 0x20;
	};

	tc->edi = tc->esi = 0;
	tc->eax = tc->ebx = tc->ecx = tc->edx = 0;
	tc->vectorNo = 0;
	tc->flags = 0;

	tc->eip = (ubit32)entryPoint;
	tc->eflags = 0;
	tc->esp = (ubit32)task->stack0
		+ (PAGING_BASE_SIZE * CHIPSET_MEMORY___KSTACK_NPAGES);

	return ERROR_SUCCESS;
}

