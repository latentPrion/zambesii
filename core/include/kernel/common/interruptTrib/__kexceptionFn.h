#ifndef _ISR_FUNCTION_T_H
	#define _ISR_FUNCTION_T_H

	#include <arch/taskContext.h>

typedef status_t (__kexceptionFn)(struct registerContextC *regs, ubit8 postcall);

#endif

