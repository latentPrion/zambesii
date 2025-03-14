
#include <config.h>
#include <arch/cpuControl.h>
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
	uarch_t	nTries = DEADLOCK_WRITE_MAX_NTRIES;
	uarch_t contenderFlags=0;

	if (cpuControl::interruptsEnabled())
	{
		FLAG_SET(contenderFlags, Lock::FLAGS_IRQS_WERE_ENABLED);
		cpuControl::disableInterrupts();
	};

#if __SCALING__ >= SCALING_SMP
	ARCH_ATOMIC_WAITLOCK_HEADER(&lock, 1, 0)
	{
		cpuControl::subZero();
		if (nTries-- <= 1) { break; };
	};

#ifdef CONFIG_DEBUG_LOCKS
	if (nTries <= 1)
	{
		cpu_t	cid;

		cid = cpuTrib.getCurrentCpuStream()->cpuId;
		if (cid == CPUID_INVALID) { cid = 0; };

		/**	EXPLANATION:
		 * This "inUse" feature allows us to detect infinite recursion
		 * deadlock loops. This can occur when the kernel somehow
		 * manages to deadlock in printf() AND also then deadlock
		 * in the deadlock debugger printf().
		 **/
		if (deadlockBuffers[cid].inUse == 1)
			{ panic(); };

		deadlockBuffers[cid].inUse = 1;

		printf(
			&deadlockBuffers[cid].buffer,
			DEADLOCK_BUFF_MAX_NBYTES,
			FATAL"Waitlock::acquire deadlock detected: nTriesRemaining: %d.\n"
			"\tCPU: %d, Lock obj addr: %p. Calling function: %p,\n"
			"\tlock int addr: %p, lockval: %d.\n",
			nTries, cid, this, __builtin_return_address(0), &lock, lock);

		deadlockBuffers[cid].inUse = 0;
		cpuControl::halt();
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

