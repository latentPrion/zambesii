
#include <chipset/pkg/chipsetPackage.h>
#include <__kstdlib/__kclib/string.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/task.h>
#include <kernel/common/cpuTrib/cpuStream.h>
#include <kernel/common/cpuTrib/powerOperations.h>
#include <__kthreads/__kcpuPowerOn.h>

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
}

status_t cpuStreamC::powerControl(ubit16 command, uarch_t flags)
{
	__kprintf(NOTICE CPUSTREAM"%d: POWER_ON received, this=0x%p, sstack=0x%p, __kcpu...Self=0x%p, sizeof cstrm 0x%x.\n",
		cpuId, this, &this->sleepStack[4096], __kcpuPowerOnBlock.cpuStream, sizeof(cpuStreamC));

	__kcpuPowerOnBlock.lock.acquire();
	__kcpuPowerOnBlock.cpuStream = this;
	__kcpuPowerOnBlock.sleepStack = &this->sleepStack[PAGING_BASE_SIZE];

	// Call the chipset code to now actually wake the CPU up.
	return (*chipsetPkg.cpus->powerControl)(cpuId, command, flags);
}

error_t cpuStreamC::initialize(void)
{
	return ERROR_SUCCESS;
}

cpuStreamC::~cpuStreamC(void)
{
}


