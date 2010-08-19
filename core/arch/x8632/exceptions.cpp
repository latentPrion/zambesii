
#include "exceptions.h"
#include <__kclasses/debugPipe.h>

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

