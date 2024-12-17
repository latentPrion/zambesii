
#include "exceptions.h"
#include <__kclasses/debugPipe.h>

status_t x8632_div_zero(RegisterContext *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_nmi(RegisterContext *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_int_overflow(RegisterContext *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_bound(RegisterContext *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_invalid_opcode(RegisterContext *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_no_fpu(RegisterContext *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_double_fault(RegisterContext *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_fpu_segfault(RegisterContext *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_tss_except(RegisterContext *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_seg_not_present(RegisterContext *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_stack_fault(RegisterContext *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_unknown_interrupt(RegisterContext *, ubit8)
{
	// Is also the FPU exception vector in SMP mode.
	return ERROR_GENERAL;
}

status_t x8632_fpu_fault(RegisterContext *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_alignment_check(RegisterContext *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_machine_check(RegisterContext *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_reserved_vector(RegisterContext *, ubit8)
{
	printf(WARNING"Interrupt came in on reserved x86-32 vector.\n");
	return ERROR_SUCCESS;
}

