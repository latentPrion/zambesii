
#include <asm/cpuControl.h>
#include <__kstdlib/__kflagManipulation.h>
#include <kernel/common/task.h>
#include <kernel/common/waitLock.h>
#include <kernel/common/cpuTrib/cpuTrib.h>

void waitLockC::acquire(void)
{
	taskC	*task;

	if (cpuControl::interruptsEnabled())
	{
		__KFLAG_SET(lockC::flags, LOCK_FLAGS_IRQS_WERE_ENABLED);
		cpuControl::disableInterrupts();
	};

#if __SCALING__ >= SCALING_SMP
	ARCH_ATOMIC_WAITLOCK_HEADER(&lock, 1, 0)
	{
		cpuControl::subZero();
	};
#endif

	task = cpuTrib.getCurrentCpuStream()->taskStream.currentTask;
	task->nLocksHeld++;
}

void waitLockC::release(void)
{
	taskC	*task;
#if __SCALING__ >= SCALING_SMP
	uarch_t		enableIrqs=0;
#endif

	if (__KFLAG_TEST(flags, LOCK_FLAGS_IRQS_WERE_ENABLED))
	{
		__KFLAG_UNSET(flags, LOCK_FLAGS_IRQS_WERE_ENABLED);
#if __SCALING__ >= SCALING_SMP
		enableIrqs = 1;
	};

	atomicAsm::set(&lock, 0);

	if (enableIrqs) {
#endif
		cpuControl::enableInterrupts();
	};

	task = cpuTrib.getCurrentCpuStream()->taskStream.currentTask;
	task->nLocksHeld--;

}

void waitLockC::releaseNoIrqs(void)
{
	taskC	*task;

	__KFLAG_UNSET(flags, LOCK_FLAGS_IRQS_WERE_ENABLED);
#if __SCALING__ >= SCALING_SMP
	atomicAsm::set(&lock, 0);
#endif

	task = cpuTrib.getCurrentCpuStream()->taskStream.currentTask;
	task->nLocksHeld--;
}

