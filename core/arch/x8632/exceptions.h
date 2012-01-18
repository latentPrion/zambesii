#ifndef x86_32_EXCEPTIONS_H
	#define x86_32_EXCEPTIONS_H

	#include <kernel/common/interruptTrib/zkcmIsrFn.h>

#ifdef __cplusplus
extern "C" {
#endif

__kexceptionFn	x8632_div_zero;
__kexceptionFn	x8632_debug;
__kexceptionFn	x8632_nmi;
__kexceptionFn	x8632_breakpoint;
__kexceptionFn	x8632_int_overflow;
__kexceptionFn	x8632_bound;
__kexceptionFn	x8632_invalid_opcode;
__kexceptionFn	x8632_no_fpu;
__kexceptionFn	x8632_double_fault;
__kexceptionFn	x8632_fpu_segfault;
__kexceptionFn	x8632_tss_except;
__kexceptionFn	x8632_seg_not_present;
__kexceptionFn	x8632_stack_fault;
__kexceptionFn	x8632_gpf;
__kexceptionFn	x8632_page_fault;
__kexceptionFn	x8632_unknown_interrupt;
__kexceptionFn	x8632_fpu_fault;
__kexceptionFn	x8632_alignment_check;
__kexceptionFn	x8632_machine_check;
__kexceptionFn	x8632_reserved_vector;

#ifdef __cplusplus
}
#endif

#endif

