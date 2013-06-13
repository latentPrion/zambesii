
#include "exceptions.h"
#include <__kclasses/debugPipe.h>

status_t x8632_div_zero(registerContextC *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_nmi(registerContextC *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_int_overflow(registerContextC *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_bound(registerContextC *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_invalid_opcode(registerContextC *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_no_fpu(registerContextC *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_double_fault(registerContextC *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_fpu_segfault(registerContextC *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_tss_except(registerContextC *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_seg_not_present(registerContextC *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_stack_fault(registerContextC *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_unknown_interrupt(registerContextC *, ubit8)
{
	// Is also the FPU exception vector in SMP mode.
	return ERROR_GENERAL;
}

status_t x8632_fpu_fault(registerContextC *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_alignment_check(registerContextC *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_machine_check(registerContextC *, ubit8)
{
	return ERROR_GENERAL;
}

status_t x8632_reserved_vector(registerContextC *, ubit8)
{
	__kprintf(WARNING"Interrupt came in on reserved x86-32 vector.\n");
	return ERROR_SUCCESS;
}

