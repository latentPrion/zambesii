
#include <arch/cpuControl.h>
#include <__kstdlib/__ktypes.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>

void panic(error_t err, utf8Char *str)
{
	switch (err)
	{
	case ERROR_MEMORY_NOMEM_PHYSICAL:
		printf(FATAL"Kernel panic: ERROR_MEMORY_NOMEM_PHYSICAL.\n");
		break;
	case ERROR_MEMORY_NOMEM_VIRTUAL:
		printf(FATAL"Kernel panic: ERROR_MEMORY_NOMEM_VIRTUAL.\n");
		break;
	case ERROR_MEMORY_NOMEM:
		printf(FATAL"Kernel panic: ERROR_MEMORY_NOMEM.\n");
		break;
	case ERROR_INVALID_ARG_VAL:
		printf(FATAL"Kernel panic: ERROR_INVALIC_ARG_VAL.\n");
		break;
	case ERROR_INVALID_ARG:
		printf(FATAL"Kernel panic: ERROR_INVALID_ARG.\n");
		break;
	case ERROR_MEMORY_NOMEM_IN_CONSTRUCTOR:
		printf(FATAL"Kernel panic: "
			"ERROR_MEMORY_NOMEM_IN_CONSTRUCTOR.\n");

		break;
	case ERROR_SUCCESS:
		printf(FATAL"Kernel panic: ERROR_SUCCESS (...?).\n");
	};

	if (str != NULL) {
		printf(str);
	};

	for (;;)
	{
		cpuControl::disableInterrupts();
		cpuControl::halt();
	};
}

void panic(utf8Char *str, error_t err)
{
	panic(err, str);
}

