#ifndef x86_32_EXCEPTIONS_H
	#define x86_32_EXCEPTIONS_H

	#include <kernel/common/interruptTrib/isrFn.h>

#ifdef __cplusplus
extern "C" {
#endif

exceptionFn	x8632_div_zero;
exceptionFn	x8632_debug;
exceptionFn	x8632_nmi;
exceptionFn	x8632_breakpoint;
exceptionFn	x8632_int_overflow;
exceptionFn	x8632_bound;
exceptionFn	x8632_invalid_opcode;
exceptionFn	x8632_no_fpu;
exceptionFn	x8632_double_fault;
exceptionFn	x8632_fpu_segfault;
exceptionFn	x8632_tss_except;
exceptionFn	x8632_seg_not_present;
exceptionFn	x8632_stack_fault;
exceptionFn	x8632_gpf;
exceptionFn	x8632_page_fault;
exceptionFn	x8632_unknown_interrupt;
exceptionFn	x8632_fpu_fault;
exceptionFn	x8632_alignment_check;
exceptionFn	x8632_machine_check;
exceptionFn	x8632_reserved_vector;

#ifdef __cplusplus
}
#endif

#endif

