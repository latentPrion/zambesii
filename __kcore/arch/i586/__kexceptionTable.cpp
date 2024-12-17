
#include <arch/x8632/interrupts.h>
#include "exceptions.h"

__kexceptionFn		*__kexceptionTable[20] =
{
	&x8632_div_zero,
	&x8632_debug,
	&x8632_nmi,
	&x8632_breakpoint,
	&x8632_int_overflow,
	&x8632_bound,
	&x8632_invalid_opcode,
	&x8632_no_fpu,
	&x8632_double_fault,
	&x8632_fpu_segfault,
	&x8632_tss_except,
	&x8632_seg_not_present,
	&x8632_stack_fault,
	&x8632_gpf,
	&x8632_page_fault,
	&x8632_unknown_interrupt,
	&x8632_fpu_fault,
	&x8632_alignment_check,
	&x8632_machine_check,
	&x8632_reserved_vector
};

