
#include <arch/paddr_t.h>
#include <arch/paging.h>
#include <arch/walkerPageRanger.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include "exceptions.h"

status_t x8632_div_zero(taskContextS *)
{
	return ERROR_GENERAL;
}

status_t x8632_debug(taskContextS *)
{
	return ERROR_GENERAL;
}

status_t x8632_nmi(taskContextS *)
{
	return ERROR_GENERAL;
}

status_t x8632_breakpoint(taskContextS *)
{
	return ERROR_GENERAL;
}

status_t x8632_int_overflow(taskContextS *)
{
	return ERROR_GENERAL;
}

status_t x8632_bound(taskContextS *)
{
	return ERROR_GENERAL;
}

status_t x8632_invalid_opcode(taskContextS *)
{
	return ERROR_GENERAL;
}

status_t x8632_no_fpu(taskContextS *)
{
	return ERROR_GENERAL;
}

status_t x8632_double_fault(taskContextS *)
{
	return ERROR_GENERAL;
}

status_t x8632_fpu_segfault(taskContextS *)
{
	return ERROR_GENERAL;
}

status_t x8632_tss_except(taskContextS *)
{
	return ERROR_GENERAL;
}

status_t x8632_seg_not_present(taskContextS *)
{
	return ERROR_GENERAL;
}

status_t x8632_stack_fault(taskContextS *)
{
	return ERROR_GENERAL;
}

status_t x8632_gpf(taskContextS *)
{
	return ERROR_GENERAL;
}

static void *reg_cr2(void)
{
	void *ret;
	asm volatile (
		"movl	%%cr2, %0\n\t"
		: "=r" (ret));

	return ret;
}

status_t x8632_page_fault(taskContextS *regs)
{
	status_t	status;
	paddr_t		p;
	uarch_t		f;

	__kprintf(WARNING"CPU %d: Page Fault:\n\tError code %p, "
		"faulting address: %p; Entering EIP was %p.\n"
		"\tTask MemoryStream: %p.\n",
		cpuTrib.getCurrentCpuStream()->cpuId,
		regs->flags, reg_cr2(), regs->eip,
		&cpuTrib.getCurrentCpuStream()
			->currentTask->parent->
			memoryStream->vaddrSpaceStream.vaddrSpace);

	status = walkerPageRanger::lookup(
		&cpuTrib.getCurrentCpuStream()
			->currentTask->parent->
			memoryStream->vaddrSpaceStream.vaddrSpace,
		reg_cr2(), &p, &f);

	switch (status)
	{
	case WPRANGER_STATUS_BACKED:
		__kprintf(WARNING"Page is backed. "
			"Most likely COW/bad access.\n");

		break;

	case WPRANGER_STATUS_FAKEMAPPED:
		__kprintf(NOTICE"Page is fakemapped.\n");
		if (!__KFLAG_TEST(f, PAGING_LEAF_STATICDATA)) {
			__kprintf(NOTICE"non STATICDATA fakemapped.\n");
		};
		break;

	case WPRANGER_STATUS_SWAPPED:
		__kprintf(NOTICE"Page is swapped out to disk.\n");
		break;

	case WPRANGER_STATUS_GUARDPAGE:
		__kprintf(NOTICE"Page is a guard page. Touch extension request.\n");
		break;

	case WPRANGER_STATUS_UNMAPPED:
		__kprintf(ERROR"Page is completely unmapped.\n");
		panic(FATAL"Unmapped page.\n");
		break;

	default:
		panic(FATAL"Unknown page status.\n");
		break;
	};

	return ERROR_SUCCESS;
}

status_t x8632_unknown_interrupt(taskContextS *)
{
	return ERROR_GENERAL;
}

status_t x8632_fpu_fault(taskContextS *)
{
	return ERROR_GENERAL;
}

status_t x8632_alignment_check(taskContextS *)
{
	return ERROR_GENERAL;
}

status_t x8632_machine_check(taskContextS *)
{
	return ERROR_GENERAL;
}

status_t x8632_reserved_vector(taskContextS *)
{
	__kprintf(WARNING"Interrupt came in on reserved x86-32 vector.\n");
	return ERROR_SUCCESS;
}

