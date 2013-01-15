
#include <chipset/memory.h>
#include <chipset/zkcm/zkcmCore.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kclasses/debugPipe.h>
#include <__kthreads/__korientation.h>
#include <kernel/common/process.h>
#include <kernel/common/task.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/cpuTrib/cpuStream.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/timerTrib/timerTrib.h>
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
	if (!__KFLAG_TEST(flags, CPUSTREAM_FLAGS_BSP)) { flags = 0; };
}

error_t cpuStreamC::initializeBspCpuLocking(void)
{
	processTrib.__kprocess.tasks[0] = &__korientationThread;
	processTrib.__kprocess.id = 0;
	// processTrib.__kprocess.absName = CC":ekfs/zambesii.zxe";
	// processTrib.__kprocess.argString = __KNULL;
	// processTrib.__kprocess.env = __KNULL;
	processTrib.__kprocess.memoryStream = &memoryTrib.__kmemoryStream;
	processTrib.__kprocess.timerStream = &timerTrib.__ktimerStream;

	// Init __korientation thread.
	memset(&__korientationThread, 0, sizeof(__korientationThread));

	__korientationThread.id = 0x0;
	__korientationThread.parent = &processTrib.__kprocess;
	__korientationThread.stack0 = __korientationStack;
	__korientationThread.nLocksHeld = 0;
	// Init cpuConfig and numaConfig BMPs later.
	__korientationThread.defaultMemoryBank.rsrc =
		CHIPSET_MEMORY_NUMA___KSPACE_BANKID;

	processTrib.__kprocess.initMagic = PROCESS_INIT_MAGIC;

	// Set the BSP's currentTask to __korientation.
	taskStream.currentTask = &__korientationThread;

	// Let the CPU know that it is the BSP.
	bspCpu.magic = CPUSTREAM_MAGIC;
	__KFLAG_SET(bspCpu.flags, CPUSTREAM_FLAGS_BSP);

	baseInit();
	return ERROR_SUCCESS;
}

error_t cpuStreamC::initializeBspCpuTaskStream(void)
{
	error_t		ret;

	/**	EXPLANATION:
	 * Called at boot to enable co-operative level scheduling on the BSP's
	 * Task Stream. Does not touch the BSP CPU's CPU Stream -- only the
	 * Task Stream member object.
	 **/
	ret = bspCpu.taskStream.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	return bspCpu.taskStream.cooperativeBind();
}

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

