
#include <arch/paging.h>
#include <arch/registerContext.h>
#include <arch/x8632/cpuFlags.h>
#include <chipset/memory.h>
#include <kernel/common/process.h>
#include <kernel/common/panic.h>

void RegisterContext::dump(void)
{
	printf(NOTICE"Context %p: dumping: [Vector num %d]:\n"
		"\t[SS %x, ESP %x]; EFLAGS %x, CS %x, EIP %x\n"
		"\t[errorCode %x]; ds %x, es %x, fs %x, gs %x\n"
		"\tesi %x, edi %x, ebp %x; eax %x, ebx %x, ecx %x, edx %x\n",
		this, vectorNo,
		ss, esp, eflags, cs, eip,
		errorCode,
		ds, es, fs, gs,
		esi, edi, ebp,
		eax, ebx, ecx, edx);
}

RegisterContext::RegisterContext(ubit8 execDomain)
{
	memset(this, 0, sizeof(*this));

	/* Set the x86 EFLAGS.IF to enable IRQs when new task is pulled.
	 *
	 **	CAVEAT:
	 * Only however, set it if this register context is being initialized
	 * when the kernel has initialized IRQ handling, or else the newly
	 * spawned thread will begin executing with IRQs enabled, and fail
	 * immediately upon executing.
	 **/
	//eflags |= x8632_CPUFLAGS_IF;
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

void RegisterContext::setStacks(
	void *stack0, void *stack1, ubit8 initialStackIndex
	)
{
	if (initialStackIndex > 1) {
		panic(CC"RegisterContext::setStacks(): illegal stack index.\n");
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

