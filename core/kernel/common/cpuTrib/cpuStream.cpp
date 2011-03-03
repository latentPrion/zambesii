
#include <__kstdlib/__kclib/string.h>
#include <kernel/common/task.h>
#include <kernel/common/cpuTrib/cpuStream.h>

// We make a global cpuStream for the bspCpu.
cpuStreamC		bspCpu(0, 0);

error_t cpuStreamC::initialize(void)
{
	return ERROR_SUCCESS;
}

cpuStreamC::~cpuStreamC(void)
{
}

