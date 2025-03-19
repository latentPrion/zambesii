
#include <config.h>
#include <arch/cpuControl.h>
#include <arch/atomic.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/thread.h>
#include <kernel/common/panic.h>
#include <kernel/common/waitLock.h>
#include <kernel/common/sharedResourceGroup.h>
#include <kernel/common/deadlock.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


void WaitLock::acquire(void)
{
#ifdef CONFIG_DEBUG_LOCKS
	// Scale the number of tries based on the number of CPUs
	cpu_t highestCpuId = atomicAsm::read(&CpuStream::highestCpuId);
	uarch_t nTries = DEADLOCK_WRITE_BASE_MAX_NTRIES +
		(highestCpuId * DEADLOCK_PER_CPU_EXTRA_WRITE_NTRIES);
#endif
	uarch_t contenderFlags=0;

	if (cpuControl::interruptsEnabled())
	{
		FLAG_SET(contenderFlags, Lock::FLAGS_IRQS_WERE_ENABLED);
		cpuControl::disableInterrupts();
	};

#if __SCALING__ >= SCALING_SMP
	ARCH_ATOMIC_WAITLOCK_HEADER(&lock, 1, 0)
	{
#ifdef CONFIG_RT_KERNEL_IRQS
		// Re-enable interrupts while we spin if they were enabled before
		if (FLAG_TEST(contenderFlags, Lock::FLAGS_IRQS_WERE_ENABLED))
			{ cpuControl::enableInterrupts(); }
#endif
		// Relax the CPU
		cpuControl::subZero();

#ifdef CONFIG_RT_KERNEL_IRQS
		// Disable interrupts again before next attempt
		cpuControl::disableInterrupts();
#endif

#ifdef CONFIG_DEBUG_LOCKS
		if (nTries-- <= 1) { break; };
#endif
	};

#ifdef CONFIG_DEBUG_LOCKS
	if (nTries <= 1)
	{
		reportDeadlock(
			FATAL"WaitLock::acquire deadlock detected:\n"
			"\tnTriesRemaining: %d, lock int addr: %p, lockval: %x\n"
			"\tCPU: %d, Lock obj addr: %p, Calling function: %p",
			nTries, &lock, lock,
			cpuTrib.getCurrentCpuStream()->cpuId, this, __builtin_return_address(0));
	};
#endif
#endif

	flags |= contenderFlags;
}

void WaitLock::release(void)
{
#if __SCALING__ >= SCALING_SMP
	uarch_t		enableIrqs=0;
#endif

	if (FLAG_TEST(flags, Lock::FLAGS_IRQS_WERE_ENABLED))
	{
		FLAG_UNSET(flags, Lock::FLAGS_IRQS_WERE_ENABLED);
#if __SCALING__ >= SCALING_SMP
		enableIrqs = 1;
	};

	atomicAsm::set(&lock, 0);

	if (enableIrqs) {
#endif
		cpuControl::enableInterrupts();
	};
}

void WaitLock::releaseNoIrqs(void)
{
	FLAG_UNSET(flags, Lock::FLAGS_IRQS_WERE_ENABLED);
#if __SCALING__ >= SCALING_SMP
	atomicAsm::set(&lock, 0);
#endif
}

