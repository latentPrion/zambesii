
#include <asm/cpuControl.h>
#include <__kstdlib/__ktypes.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>

void panic(error_t err, utf8Char *str)
{
	switch (err)
	{
	case ERROR_MEMORY_NOMEM_PHYSICAL:
		__kprintf(FATAL"Kernel panic: ERROR_MEMORY_NOMEM_PHYSICAL.\n");
		break;
	case ERROR_MEMORY_NOMEM_VIRTUAL:
		__kprintf(FATAL"Kernel panic: ERROR_MEMORY_NOMEM_VIRTUAL.\n");
		break;
	case ERROR_MEMORY_NOMEM:
		__kprintf(FATAL"Kernel panic: ERROR_MEMORY_NOMEM.\n");
		break;
	case ERROR_INVALID_ARG_VAL:
		__kprintf(FATAL"Kernel panic: ERROR_INVALIC_ARG_VAL.\n");
		break;
	case ERROR_INVALID_ARG:
		__kprintf(FATAL"Kernel panic: ERROR_INVALID_ARG.\n");
		break;
	case ERROR_MEMORY_NOMEM_IN_CONSTRUCTOR:
		__kprintf(FATAL"Kernel panic: "
			"ERROR_MEMORY_NOMEM_IN_CONSTRUCTOR.\n");

		break;
	case ERROR_SUCCESS:
		__kprintf(FATAL"Kernel panic: ERROR_SUCCESS (...?).\n");
	};

	if (str != __KNULL) {
		__kprintf(str);
	};

	for (;;)
	{
		cpuControl::halt();
		cpuControl::disableInterrupts();
	};
}

void panic(utf8Char *str, error_t err)
{
	panic(err, str);
}

