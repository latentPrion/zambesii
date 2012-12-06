
#include <chipset/zkcm/zkcmCore.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/task.h>
#include <kernel/common/cpuTrib/cpuStream.h>
#include <kernel/common/cpuTrib/powerOperations.h>
#include <__kthreads/__kcpuPowerOn.h>

// We make a global cpuStream for the bspCpu.
#if __SCALING__ >= SCALING_CC_NUMA
cpuStreamC		bspCpu(0, 0, 0);
#else
cpuStreamC		bspCpu(0, 0);
#endif


/**	NOTE:
 * A lot of preprocessing in here: It looks quite ugly I suppose.
 **/
#if __SCALING__ >= SCALING_CC_NUMA
cpuStreamC::cpuStreamC(numaBankId_t bid, cpu_t cid, ubit32 acpiId)
#else
cpuStreamC::cpuStreamC(cpu_t cid, ubit32 acpiId)
#endif
:
cpuId(cid), cpuAcpiId(acpiId),
#if __SCALING__ >= SCALING_CC_NUMA
bankId(bid),
#endif
taskStream(this)
#if __SCALING__ >= SCALING_SMP
,interCpuMessager(this)
#endif
{
	/* Only the BSP CPU Stream will have this magic value set. The BSP
	 * CPU Stream is pre-initialized, and holds certain values already which
	 * should not be overwritten.
	 **/
	if (magic != CPUSTREAM_MAGIC) { flags = 0; };

	memset(&cpuFeatures, 0, sizeof(cpuFeatures));
	memset(&archBlock, 0, sizeof(archBlock));
}

#include <arch/tlbControl.h>

status_t cpuStreamC::powerControl(ubit16 command, uarch_t flags)
{

	__kcpuPowerOnBlock.lock.acquire();
	__kcpuPowerOnBlock.cpuStream = this;
	__kcpuPowerOnBlock.sleepStack = &sleepStack[PAGING_BASE_SIZE];

	// Call the chipset code to now actually wake the CPU up.
	zkcmCore.cpuDetection.powerControl(cpuId, command, flags);

	return ERROR_SUCCESS;
}

cpuStreamC::~cpuStreamC(void)
{
}

