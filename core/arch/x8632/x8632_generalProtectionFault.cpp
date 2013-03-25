
#include <arch/cpuControl.h>
#include <arch/taskContext.h>
#include <__kclasses/debugPipe.h>
#include "exceptions.h"

status_t x8632_gpf(taskContextC *regs, ubit8)
{
	__kprintf(NOTICE NOLOG"#GPF: Culprit selector 0x%x. Halting.\n",
		regs->errorCode);

	for (;;)
	{
		cpuControl::disableInterrupts();
		cpuControl::halt();
	};

	return ERROR_SUCCESS;
}

