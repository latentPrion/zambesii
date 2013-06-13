#include <arch/arch_include.h>
#include ARCH_INCLUDE(taskContext.h)

extern "C"
{
	void saveContextAndCallPull(void *cpuSleepStackBaseAddr);
	void loadContextAndJump(registerContextC *tc);
}

