
#include <chipset/memory.h>
#include <chipset/zkcm/zkcmCore.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kclasses/debugPipe.h>
#include <__kthreads/__korientation.h>
#include <kernel/common/process.h>
#include <kernel/common/thread.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/cpuTrib/cpuStream.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/cpuTrib/powerOperations.h>
#include <__kthreads/__kcpuPowerOn.h>


// We make a global CpuStream for the bspCpu.
#if __SCALING__ >= SCALING_CC_NUMA
CpuStream		bspCpu(NUMABANKID_INVALID, CPUID_INVALID, 0);
#elif __SCALING__ == SCALING_SMP
CpuStream		bspCpu(CPUID_INVALID, 0);
#else
CpuStream		bspCpu(0, 0);
#endif

extern "C" ubit8	*const bspCpuPowerStack = bspCpu.powerStack;

/**	NOTE:
 * A lot of preprocessing in here: It looks quite ugly I suppose.
 **/
#if __SCALING__ >= SCALING_CC_NUMA
CpuStream::CpuStream(numaBankId_t bid, cpu_t cid, ubit32 acpiId)
#else
CpuStream::CpuStream(cpu_t cid, ubit32 acpiId)
#endif
:
cpuId(cid), cpuAcpiId(acpiId),
#if __SCALING__ >= SCALING_CC_NUMA
bankId(bid),
#endif
taskStream(this), flags(0),
powerManager(this),
#if __SCALING__ >= SCALING_SMP
interCpuMessager(this),
#endif
#if defined(CONFIG_ARCH_x86_32) || defined(CONFIG_ARCH_x86_64)
lapic(this),
#endif
powerThread(cpuId, processTrib.__kgetStream(), NULL)
{
	if (this == &bspCpu) { FLAG_SET(flags, CPUSTREAM_FLAGS_BSP); };

	/* We don't need to acquire the __kcpuPowerStacksLock lock before
	 * editing this CPU's index in the array, because a race condition can
	 * never occur here. No other writer will ever write to any other CPU's
	 * index, so the operation of writing a sleep stack pointer to the
	 * array is always serial in nature.
	 *
	 *	FIXME:
	 * Notice that we do not allow the BSP CPU to set its index in the
	 * sleepstacks array. When writing power management, if it becomes
	 * necessary for the BSP CPU to know its sleepstack location, we may
	 * need to change this behaviour, or else just explicitly set the
	 * BSP CPU's sleep stack in the power management code.
	 **/
	if (!FLAG_TEST(flags, CPUSTREAM_FLAGS_BSP))
	{
		__kcpuPowerStacks[cpuId] = &powerStack[
			sizeof(powerStack) - sizeof(void *)];

		/* Push the CPU Stream address to the sleep stack so the waking
		 * CPU can obtain it as if it was passed as an argument to
		 * __kcpuPowerOnMain.
		 **/
		*reinterpret_cast<CpuStream **>(
			__kcpuPowerStacks[cpuId] ) = this;
	};
}

error_t CpuStream::initializeBspCpuLocking(void)
{
	/**	EXPLANATION:
	 * This gets called before global constructors get called. In here we
	 * set the CPUSTREAM_FLAGS_BSP flag on the BSP CPU Stream so that when
	 * its constructor is executed it doesn't trample itself.
	 **/
	__korientationThread.nLocksHeld = 0;
	__korientationThread.parent = processTrib.__kgetStream();
	// Init cpuConfig and numaConfig BMPs later.
	__korientationThread.defaultMemoryBank.rsrc =
		CHIPSET_NUMA___KSPACE_BANKID;

	// Let the CPU know that it is the BSP.
	FLAG_SET(flags, CPUSTREAM_FLAGS_BSP);
	// Set the BSP's currentTask to __korientation.
	taskStream.currentThread = &__korientationThread;

	baseInit();

	return ERROR_SUCCESS;
}

error_t CpuStream::initialize(void)
{
	error_t		ret;

	ret = powerThread.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	ret = interCpuMessager.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

#if defined(CONFIG_ARCH_x86_32) || defined(CONFIG_ARCH_x86_64)
	ret = lapic.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };
#endif

	return taskStream.initialize();
}

void CpuStream::cut(void)
{
	// Probably won't need to do much here.
}

CpuStream::~CpuStream(void)
{
}

