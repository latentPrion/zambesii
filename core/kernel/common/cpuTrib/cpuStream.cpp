
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


// We make a global cpuStream for the bspCpu.
#if __SCALING__ >= SCALING_CC_NUMA
cpuStreamC		bspCpu(NUMABANKID_INVALID, CPUID_INVALID, 0);
#elif __SCALING__ == SCALING_SMP
cpuStreamC		bspCpu(CPUID_INVALID, 0);
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
taskStream(this), flags(0),
powerManager(this),
#if __SCALING__ >= SCALING_SMP
interCpuMessager(this),
#endif
#if defined(CONFIG_ARCH_x86_32) || defined(CONFIG_ARCH_x86_64)
lapic(this),
#endif
perCpuTaskContext(task::PER_CPU, this)
{
	if (this == &bspCpu) { FLAG_SET(flags, CPUSTREAM_FLAGS_BSP); };

	/* We don't need to acquire the __kcpuPowerOnSleepStacksLock lock before
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
		__kcpuPowerOnSleepStacks[cpuId] = &perCpuThreadStack[
			sizeof(perCpuThreadStack) - sizeof(void *)];

		/* Push the CPU Stream address to the sleep stack so the waking
		 * CPU can obtain it as if it was passed as an argument to
		 * __kcpuPowerOnMain.
		 **/
		*reinterpret_cast<cpuStreamC **>( 
			__kcpuPowerOnSleepStacks[cpuId] ) = this;
	};
}

error_t cpuStreamC::initializeBspCpuLocking(void)
{
	/**	EXPLANATION:
	 * This gets called before global constructors get called. In here we
	 * set the CPUSTREAM_FLAGS_BSP flag on the BSP CPU Stream so that when
	 * its constructor is executed it doesn't trample itself.
	 **/
	__korientationThread.getTaskContext()->nLocksHeld = 0;
	__korientationThread.parent = processTrib.__kgetStream();
	// Init cpuConfig and numaConfig BMPs later.
	__korientationThread.getTaskContext()->defaultMemoryBank.rsrc =
		CHIPSET_NUMA___KSPACE_BANKID;

	// Let the CPU know that it is the BSP.
	FLAG_SET(flags, CPUSTREAM_FLAGS_BSP);
	// Set the BSP's currentTask to __korientation.
	taskStream.currentTask = &__korientationThread;

	baseInit();

	return ERROR_SUCCESS;
}

error_t cpuStreamC::initialize(void)
{
	error_t		ret;

	ret = interCpuMessager.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

#if defined(CONFIG_ARCH_x86_32) || defined(CONFIG_ARCH_x86_64)
	ret = lapic.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };
#endif

	return taskStream.initialize();
}

void cpuStreamC::cut(void)
{
	// Probably won't need to do much here.
}

cpuStreamC::~cpuStreamC(void)
{
}

