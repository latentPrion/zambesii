
#include <scaling.h>
#include <arch/cpuControl.h>
#include <__kstdlib/__kflagManipulation.h>
#include <kernel/common/recursiveLock.h>
#include <kernel/common/thread.h>
#include <kernel/common/processId.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <arch/x8632/atomic.h>
#include <kernel/common/panic.h>
#include <kernel/common/deadlock.h>

/**	EXPLANATION:
 * On a non-SMP build, there is only ONE processor, so there is no need to
 * spin on a lock. Also, there's no need to check the thread ID since only
 * one thread can run at once. And that thread will not be able to leave the
 * CPU until it leaves the critical section. Therefore, we can just
 * disable IRQs and increment the recursionCount member on acquire and
 * decrement it on release.
 **/

void RecursiveLock::acquire(void)
{
	Thread		*thread;
	processId_t	currThreadId, oldValue;
	uarch_t		irqsWereEnabled;
#ifdef CONFIG_DEBUG_LOCKS
	// Scale the number of tries based on the number of CPUs
	cpu_t highestCpuId = atomicAsm::read(&CpuStream::highestCpuId);
	uarch_t nTries = DEADLOCK_WRITE_BASE_MAX_NTRIES +
		(highestCpuId * DEADLOCK_PER_CPU_EXTRA_WRITE_NTRIES);
#endif

	thread = cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentThread();
	currThreadId = thread->getFullId();

	// Save the current interrupt state
	irqsWereEnabled = cpuControl::interruptsEnabled();

	// Disable interrupts to ensure atomicity
	cpuControl::disableInterrupts();

#if __SCALING__ >= SCALING_SMP
	for (;FOREVER;)
	{
		// Try to acquire the lock if it's free (PROCID_INVALID)
		oldValue = atomicAsm::compareAndExchange(
			&lock,
			PROCID_INVALID, currThreadId);

		if (oldValue == PROCID_INVALID)
		{
			// We acquired the lock
			recursionCount = 1;
			if (irqsWereEnabled)
				{ FLAG_SET(flags, Lock::FLAGS_IRQS_WERE_ENABLED); }
			return;
		}
		else if (oldValue == currThreadId)
		{
			// We already own the lock, increment recursion count
			recursionCount++;
			return;
		}

		/* If we get here, someone else owns the lock
		 * Re-enable interrupts while we spin
		 * */
		if (irqsWereEnabled)
			{ cpuControl::enableInterrupts(); }

		// Relax the CPU.
		cpuControl::subZero();
		
		// Disable interrupts again before next attempt
		cpuControl::disableInterrupts();

#ifdef CONFIG_DEBUG_LOCKS
		if (nTries-- <= 1)
		{
			reportDeadlock(
				FATAL"RecursiveLock::acquire deadlock detected:\n"
				"\tnTriesRemaining: %d, lock int addr: %p, lockval: %x\n"
				"\tcurrThreadId: %x, CPU: %d, Lock obj addr: %p, Calling function: %p",
				nTries, &lock, lock, currThreadId,
				cpuTrib.getCurrentCpuStream()->cpuId, this, __builtin_return_address(0));
		}
#endif
	}
#else
	// In non-SMP mode, just take the lock
	lock = currThreadId;
	recursionCount++;
	if (irqsWereEnabled)
		{ FLAG_SET(flags, Lock::FLAGS_IRQS_WERE_ENABLED); }
#endif
}

void RecursiveLock::release(void)
{
	/* We make the assumption that a task will only call release() if it
	 * already holds the lock. this is pretty logical. Anyone who does
	 * otherwise is simply being malicious and stupid.
	 **/

	recursionCount--;
	
	// If we're releasing the lock completely, open it up for other tasks
	if (recursionCount == 0)
	{
		uarch_t irqsWereEnabled = FLAG_TEST(flags, Lock::FLAGS_IRQS_WERE_ENABLED);
		FLAG_UNSET(flags, Lock::FLAGS_IRQS_WERE_ENABLED);
		
		// Mark the lock as free atomically
		atomicAsm::set(&lock, PROCID_INVALID);
		
		// Restore interrupts if they were enabled before
		if (irqsWereEnabled)
			{ cpuControl::enableInterrupts(); }
	}
}
