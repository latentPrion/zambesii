
#include <asm/cpuControl.h>
#include <__kstdlib/__ktypes.h>
#include <kernel/common/panic.h>

void panic(error_t err)
{
	switch (err)
	{
		case ERROR_MEMORY_NOMEM_PHYSICAL: break;
		case ERROR_MEMORY_NOMEM_VIRTUAL: break;
		case ERROR_MEMORY_NOMEM: break;
		case ERROR_INVALID_ARG_VAL: break;
		case ERROR_INVALID_ARG: break;
		case ERROR_MEMORY_NOMEM_IN_CONSTRUCTOR: break;
	};
	for (;;)
	{
		cpuControl::halt();
		cpuControl::disableInterrupts();
	};
}
void panic(const LANG_TYPE)
{
	for (;;)
	{
		cpuControl::halt();
		cpuControl::disableInterrupts();
	};
}

