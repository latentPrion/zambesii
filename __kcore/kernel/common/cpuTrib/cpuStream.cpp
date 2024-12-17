
#include <chipset/memory.h>
#include <chipset/zkcm/zkcmCore.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kclasses/debugPipe.h>
#include <__kthreads/main.h>
#include <kernel/common/process.h>
#include <kernel/common/thread.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/cpuTrib/cpuStream.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/cpuTrib/powerOperations.h>
#include <__kthreads/__kcpuPowerOn.h>


bspPlugTypeE		CpuStream::bspPlugType = BSP_PLUGTYPE_FIRSTPLUG;
cpu_t			CpuStream::bspCpuId = CPUID_INVALID;
ubit32			CpuStream::bspAcpiId = CPUID_INVALID;
numaBankId_t		CpuStream::bspBankId = NUMABANKID_INVALID;
sCpuFeatures		CpuStream::baseCpuFeatures;

/**	NOTE:
 * We *MUST* force the bspCpu object to go into the .data section, and it
 * *MUSTN'T* end up in the .bss section.
 *
 * This is because we the bspCpu object contains the stack used by the BSP CPU.
 * And because we memset(0) the entire .bss section AFTER the BSP has already
 * begun using its this same stack!
 **/
__attribute__((section(".data")))
// We make a global CpuStream for the bspCpu.
#if __SCALING__ >= SCALING_CC_NUMA
CpuStream		bspCpu(NUMABANKID_INVALID, CPUID_INVALID, 0);
#elif __SCALING__ == SCALING_SMP
CpuStream		bspCpu(CPUID_INVALID, 0);
#else
// Not sure which constructor this is supposed to be calling.
CpuStream		bspCpu(0, 0);
#endif

extern "C" ubit8	*const bspCpuPowerStack = bspCpu.taskStream.powerStack;

/**	NOTE:
 * A lot of preprocessing in here: It looks quite ugly I suppose.
 **/
CpuStream::CpuStream(
#if __SCALING__ >= SCALING_CC_NUMA
	numaBankId_t bid,
#endif
	cpu_t cid, ubit32 acpiId
	)
:
cpuId(cid), cpuAcpiId(acpiId),
#if __SCALING__ >= SCALING_CC_NUMA
bankId(bid),
#endif
taskStream(cid, this), flags(0),
powerManager(cid, this),
#if __SCALING__ >= SCALING_SMP
interCpuMessager(cid, this),
#endif
#if defined(CONFIG_ARCH_x86_32) || defined(CONFIG_ARCH_x86_64)
lapic(this)
#endif
{
}

error_t CpuStream::initialize(void)
{
	error_t		ret;

	ret = interCpuMessager.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

#if defined(CONFIG_ARCH_x86_32) || defined(CONFIG_ARCH_x86_64)
	ret = lapic.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };
#endif

	ret = taskStream.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	ret = powerManager.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	/* We don't need to acquire the __kcpuPowerStacksLock lock before
	 * editing this CPU's index in the array, because a race condition can
	 * never occur here. No other writer will ever write to any other CPU's
	 * index, so the operation of writing a sleep stack pointer to the
	 * array is always serial in nature.
	 *
	 *	FIXME:
	 * Notice that we do not allow the BSP CPU to set its index on its first
	 * "plug". When writing power management, if it becomes
	 * necessary for the BSP CPU to know its sleepstack location, we may
	 * need to change this behaviour, or else just explicitly set the
	 * BSP CPU's sleep stack in the power management code.
	 **/
	if (!isBspFirstPlug())
	{
		__kcpuPowerStacks[cpuId] = &taskStream.powerStack[
			sizeof(taskStream.powerStack) - sizeof(void *)];

		/* Push the CPU Stream address to the power stack so the waking
		 * CPU can obtain it as if it was passed as an argument to
		 * __kcpuPowerOnMain.
		 **/
		*reinterpret_cast<CpuStream **>(
			__kcpuPowerStacks[cpuId] ) = this;
	};

	return ERROR_SUCCESS;
}

void CpuStream::cut(void)
{
	// Probably won't need to do much here.
}

CpuStream::~CpuStream(void)
{
}

