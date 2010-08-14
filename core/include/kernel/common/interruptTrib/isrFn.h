#ifndef _ISR_FUNCTION_T_H
	#define _ISR_FUNCTION_T_H

	#include <arch/taskContext.h>

typedef status_t (isrFn)(void *persistentScratch);
typedef status_t (exceptionFn)(struct taskContextS *regs);

#endif

