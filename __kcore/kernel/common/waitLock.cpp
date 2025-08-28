
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
	cpu_t highestCpuId = atomicAsm::read<cpu_t>(&CpuStream::highestCpuId);
	uarch_t nTries = calcDeadlockNWriteTries(highestCpuId);
#endif
	uarch_t contenderFlags=0;

#if __SCALING__ >= SCALING_SMP && defined(CONFIG_DEBUG_LOCKS)
	if (cpuTrib.getCurrentCpuStream()->nLocksHeld > 0
		&& cpuControl::interruptsEnabled())
	{
		printf(FATAL"%s(%s): nLocksHeld=%d but local IRQs=%d\n",
			name, __func__,
			cpuTrib.getCurrentCpuStream()->nLocksHeld,
			cpuControl::interruptsEnabled());

		panic(ERROR_INVALID_STATE);
	}
#endif

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
			FATAL"WaitLock::acquire[%s] deadlock detected:\n"
			"\tnTriesRemaining: %d, lock int addr: %p, lockval: %x "
			"flags: %x\n"
			"\tCPU: %d, Lock obj addr: %p, Calling function: %p, "
			"\tcurr ownerAcquisitionInstr: %p, local IRQs: %d\n",
			name, nTries, &lock, lock, flags,
			cpuTrib.getCurrentCpuStream()->cpuId, this,
			__builtin_return_address(0),
			ownerAcquisitionInstr,
			!!cpuControl::interruptsEnabled());
	};

	ownerAcquisitionInstr = reinterpret_cast<void(*)()>(
		__builtin_return_address(0));
#endif
#endif
#ifdef CONFIG_DEBUG_LOCKED_INTERRUPT_ENTRY
	cpuTrib.getCurrentCpuStream()->nLocksHeld++;
	cpuTrib.getCurrentCpuStream()->mostRecentlyAcquiredLock = this;
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
#endif
#ifdef CONFIG_DEBUG_LOCKED_INTERRUPT_ENTRY
	cpuTrib.getCurrentCpuStream()->nLocksHeld--;
#endif
#if __SCALING__ >= SCALING_SMP

	if (enableIrqs)
	{
#ifdef CONFIG_DEBUG_LOCKS
		if (cpuTrib.getCurrentCpuStream()->nLocksHeld > 0)
		{
			printf(FATAL"%s(%s): nLocksHeld=%d but we're enabling "
				"local IRQs.\n",
				name, __func__,
				cpuTrib.getCurrentCpuStream()->nLocksHeld);

			panic(ERROR_INVALID_STATE);
		}
#endif
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
#ifdef CONFIG_DEBUG_LOCKED_INTERRUPT_ENTRY
	cpuTrib.getCurrentCpuStream()->nLocksHeld--;
#endif
}

/*
 * USAGE EXAMPLES:
 *
 * Example 1: Transfer IRQ ownership to MultipleReaderLock write lock
 * WaitLock wlock("example");
 * MultipleReaderLock mrlock("example");
 *
 * wlock.acquire();
 * mrlock.writeAcquire();
 * wlock.giveOwnershipOfLocalIrqsTo(&mrlock);
 * wlock.release();  // Won't re-enable IRQs
 * // ... critical section ...
 * mrlock.writeRelease();  // Will re-enable IRQs if they were enabled before
 *
 * Example 2: Transfer IRQ ownership to MultipleReaderLock read lock
 * WaitLock wlock("example");
 * MultipleReaderLock mrlock("example");
 * uarch_t mrFlags = 0;
 *
 * wlock.acquire();
 * mrlock.readAcquire(&mrFlags);
 * wlock.giveOwnershipOfLocalIrqsTo(&mrFlags);
 * wlock.release();  // Won't re-enable IRQs
 * // ... critical section ...
 * mrlock.readRelease(mrFlags);  // Will re-enable IRQs if they were enabled before
 */
void WaitLock::giveOwnershipOfLocalIrqsTo(MultipleReaderLock *targetLock)
{
	/**	EXPLANATION:
	 * Transfer IRQ ownership from this WaitLock to the target
	 * MultipleReaderLock. This prevents the WaitLock from re-enabling IRQs
	 * when it's released, allowing the MultipleReaderLock to maintain
	 * control of the IRQ state throughout its critical section.
	 *
	 * This is used in nested lock scenarios like:
	 * WaitLock.acquire()
	 * MRLock.acquire()
	 * WaitLock.giveOwnershipOfLocalIrqsTo(&MRLock)
	 * WaitLock.release()  // Won't re-enable IRQs
	 * // ... critical section ...
	 * MRLock.release()    	// Will re-enable IRQs if they were enabled
	 *			// before the MRLock was acquired.
	 */
	// Delegate to the uarch_t* version by accessing the internal flags
	giveOwnershipOfLocalIrqsTo(&targetLock->flags);
}

void WaitLock::giveOwnershipOfLocalIrqsTo(uarch_t *targetFlags)
{
	/**	EXPLANATION:
	 * Transfer IRQ ownership from this WaitLock to a local flags variable.
	 * This is used when the target is a MultipleReaderLock that was
	 * acquired via readAcquire(), which uses a local stack variable to
	 * track IRQ state.
	 *
	 * This is used in nested lock scenarios like:
	 * WaitLock.acquire()
	 * uarch_t mrFlags = 0;
	 * MRLock.readAcquire(&mrFlags)
	 * WaitLock.giveOwnershipOfLocalIrqsTo(&mrFlags)
	 * WaitLock.release()  // Won't re-enable IRQs
	 * // ... critical section ...
	 * MRLock.readRelease(mrFlags)  // Will re-enable IRQs if they were
	 *				// enabled before the MRLock was
	 *				// acquired.
	 */
	if (FLAG_TEST(flags, Lock::FLAGS_IRQS_WERE_ENABLED))
	{
		// Transfer the flag from WaitLock to the local flags variable
		FLAG_UNSET(flags, Lock::FLAGS_IRQS_WERE_ENABLED);
		FLAG_SET(*targetFlags, Lock::FLAGS_IRQS_WERE_ENABLED);
	}
}
