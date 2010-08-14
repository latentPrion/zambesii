
#include <__kclasses/debugPipe.h>
#include "exceptions.h"

status_t x8632_div_zero(tastContextS *regs)
{
	return ERROR_GENERAL;
}

status_t x8632_debug(tastContextS *regs)
{
	return ERROR_GENERAL;
}

status_t x8632_nmi(tastContextS *regs)
{
	return ERROR_GENERAL;
}

status_t x8632_breakpoint(tastContextS *regs)
{
	return ERROR_GENERAL;
}

status_t x8632_int_overflow(tastContextS *regs)
{
	return ERROR_GENERAL;
}

status_t x8632_bound(tastContextS *regs)
{
	return ERROR_GENERAL;
}

status_t x8632_invalid_opcode(tastContextS *regs)
{
	return ERROR_GENERAL;
}

status_t x8632_no_fpu(tastContextS *regs)
{
	return ERROR_GENERAL;
}

status_t x8632_double_fault(tastContextS *regs)
{
	return ERROR_GENERAL;
}

status_t x8632_fpu_segfault(tastContextS *regs)
{
	return ERROR_GENERAL;
}

status_t x8632_tss_except(tastContextS *regs)
{
	return ERROR_GENERAL;
}

status_t x8632_seg_not_present(tastContextS *regs)
{
	return ERROR_GENERAL;
}

status_t x8632_stack_fault(tastContextS *regs)
{
	return ERROR_GENERAL;
}

status_t x8632_gpf(tastContextS *regs)
{
	return ERROR_GENERAL;
}

status_t x8632_page_fault(tastContextS *regs)
{
	__kprintf(WARNING"Page fault.\n");
	return ERROR_SUCCESS;
}

status_t x8632_unknown_interrupt(tastContextS *regs)
{
	return ERROR_GENERAL;
}

status_t x8632_fpu_fault(tastContextS *regs)
{
	return ERROR_GENERAL;
}

status_t x8632_alignment_check(tastContextS *regs)
{
	return ERROR_GENERAL;
}

status_t x8632_machine_check(tastContextS *regs)
{
	return ERROR_GENERAL;
}

status_t x8632_reserved_vector(taskContextS *regs)
{
	__kprintf(WARNING"Interrupt came in on reserved x86-32 vector.\n");
	return ERROR_SUCCESS;
}

