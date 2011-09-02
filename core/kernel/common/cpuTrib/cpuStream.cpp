
#include <__kstdlib/__kclib/string.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/task.h>
#include <kernel/common/cpuTrib/cpuStream.h>
#include <kernel/common/cpuTrib/powerOperations.h>

// We make a global cpuStream for the bspCpu.
cpuStreamC		bspCpu(0, 0);


cpuStreamC::cpuStreamC(numaBankId_t bid, cpu_t cid)
:
cpuId(cid), bankId(bid), scheduler(this)
{
	// Nothing to be done for now.
}

sarch_t cpuStreamC::reConstruct(void)
{
	__kprintf(NOTICE"BSP reconstructing.\n");
	return 0;
};

status_t cpuStreamC::powerControl(ubit16, uarch_t)
{
	__kprintf(NOTICE CPUSTREAM"%d: POWER_ON received.\n", cpuId);
	return 0;
}

error_t cpuStreamC::initialize(void)
{
	return ERROR_SUCCESS;
}

cpuStreamC::~cpuStreamC(void)
{
}


