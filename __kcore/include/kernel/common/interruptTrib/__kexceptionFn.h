#ifndef _ISR_FUNCTION_T_H
	#define _ISR_FUNCTION_T_H

	#include <arch/registerContext.h>

typedef status_t
	(__kexceptionFn)(class RegisterContext *regs, ubit8 postcall);

#endif

