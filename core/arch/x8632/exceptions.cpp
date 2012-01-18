
#include "exceptions.h"
#include <__kclasses/debugPipe.h>

status_t x8632_div_zero(taskContextS *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_debug(taskContextS *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_nmi(taskContextS *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_breakpoint(taskContextS *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_int_overflow(taskContextS *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_bound(taskContextS *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_invalid_opcode(taskContextS *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_no_fpu(taskContextS *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_double_fault(taskContextS *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_fpu_segfault(taskContextS *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_tss_except(taskContextS *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_seg_not_present(taskContextS *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_stack_fault(taskContextS *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_gpf(taskContextS *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_unknown_interrupt(taskContextS *, ubit8)
{
	// Is also the FPU exception vector in SMP mode.
	return ERROR_GENERAL;
}

status_t x8632_fpu_fault(taskContextS *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_alignment_check(taskContextS *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_machine_check(taskContextS *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_reserved_vector(taskContextS *, ubit8)
{
	__kprintf(WARNING"Interrupt came in on reserved x86-32 vector.\n");
	return ERROR_SUCCESS;
}

