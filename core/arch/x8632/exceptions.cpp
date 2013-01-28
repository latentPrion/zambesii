
#include "exceptions.h"
#include <__kclasses/debugPipe.h>

status_t x8632_div_zero(taskContextC *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_debug(taskContextC *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_nmi(taskContextC *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_breakpoint(taskContextC *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_int_overflow(taskContextC *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_bound(taskContextC *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_invalid_opcode(taskContextC *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_no_fpu(taskContextC *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_double_fault(taskContextC *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_fpu_segfault(taskContextC *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_tss_except(taskContextC *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_seg_not_present(taskContextC *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_stack_fault(taskContextC *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_gpf(taskContextC *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_unknown_interrupt(taskContextC *, ubit8)
{
	// Is also the FPU exception vector in SMP mode.
	return ERROR_GENERAL;
}

status_t x8632_fpu_fault(taskContextC *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_alignment_check(taskContextC *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_machine_check(taskContextC *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_reserved_vector(taskContextC *, ubit8)
{
	__kprintf(WARNING"Interrupt came in on reserved x86-32 vector.\n");
	return ERROR_SUCCESS;
}

