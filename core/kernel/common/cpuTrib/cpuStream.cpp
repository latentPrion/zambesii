
#include <kernel/common/task.h>
#include <kernel/common/cpuTrib/cpuStream.h>

// We make a global cpuStream for the bspCpu.
cpuStreamC		bspCpu;

cpuStreamC::cpuStreamC(void)
{
}

cpuStreamC::cpuStreamC(cpu_t id)
:
cpuId(id)
{
}

cpuStreamC::~cpuStreamC(void)
{
}

