
#include <arch/cpuControl.h>
#include <arch/registerContext.h>
#include <__kclasses/debugPipe.h>
#include "exceptions.h"

status_t x8632_gpf(RegisterContext *regs, ubit8)
{
	printf(NOTICE OPTS(NOLOG)
		"#GPF: Culprit selector %x. Halting.\n",
		regs->errorCode);

	for (;FOREVER;)
	{
		cpuControl::disableInterrupts();
		cpuControl::halt();
	};

	return ERROR_SUCCESS;
}

