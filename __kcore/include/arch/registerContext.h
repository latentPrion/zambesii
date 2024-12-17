#include <arch/arch_include.h>
#include ARCH_INCLUDE(registerContext.h)

extern "C"
{
	void saveContextAndCallPull(void *cpuSleepStackBaseAddr);
	void loadContextAndJump(RegisterContext *tc);
}

