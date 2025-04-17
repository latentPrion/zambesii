
#include <arch/atomic.h>
#include <arch/cpuControl.h>
#include <__kstdlib/__kflagManipulation.h>
#include <kernel/common/multipleReaderLock.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/panic.h>
#include <kernel/common/deadlock.h>


/**	FIXME:
 * A series of race conditions exist in this class, mostly with the writer
 * acquire/release functions and the readReleaseWriteAcquire function.
 *
 * Get rid of these whenever next there is a freeze period.
 **/
void MultipleReaderLock::readAcquire(uarch_t *_flags)
{
	if (cpuControl::interruptsEnabled())
	{
		FLAG_SET(*_flags, Lock::FLAGS_IRQS_WERE_ENABLED);
		cpuControl::disableInterrupts();
	}

#if __SCALING__ >= SCALING_SMP
#ifdef CONFIG_DEBUG_LOCKS
	// Scale the number of tries based on the number of CPUs
	cpu_t highestCpuId = atomicAsm::read(&CpuStream::highestCpuId);
	uarch_t nReadTriesRemaining = calcDeadlockNReadTries(highestCpuId);
#endif

	for (;FOREVER;)
	{
		atomicAsm::increment(&lock);
		if (!FLAG_TEST(
			atomicAsm::read<uarch_t>(&lock),
			MR_FLAGS_WRITE_REQUEST))
		{
			break;
		}
		else
		{
			/**	EXPLANATION:
			 * If there's someone requesting write access, we need
			 * to decrement the reader count so they can acquire
			 * the lock; then we back out and try again after a
			 * pause
			 */
			atomicAsm::decrement(&lock);
#ifdef CONFIG_RT_KERNEL_IRQS
			// Re-enable interrupts while we spin if they were enabled before
			if (FLAG_TEST(*_flags, Lock::FLAGS_IRQS_WERE_ENABLED))
				{ cpuControl::enableInterrupts(); }
#endif
			cpuControl::subZero();
#ifdef CONFIG_RT_KERNEL_IRQS
			cpuControl::disableInterrupts();
#endif

#ifdef CONFIG_DEBUG_LOCKS
			if (nReadTriesRemaining-- <= 1) { break; }
#endif
		}
	}

#ifdef CONFIG_DEBUG_LOCKS
	if (nReadTriesRemaining <= 1)
	{
		uarch_t lockValue = atomicAsm::read(&lock);
		uarch_t nReaders = lockValue & ((1 << MR_FLAGS_WRITE_REQUEST_SHIFT) - 1);
		uarch_t writeRequestSet = (lockValue & MR_FLAGS_WRITE_REQUEST) != 0;

		reportDeadlock(
			FATAL"MultipleReaderLock::readAcquire[%s] deadlock detected:\n"
			"\tnReadTriesRemaining: %d, lock int addr: %p, lockval: %x\n"
			"\tWrite request bit: %s, Number of readers: %d\n"
			"\tCPU: %d, Lock obj addr: %p, Calling function: %p",
			name, nReadTriesRemaining, &lock, lock,
			writeRequestSet ? "SET" : "CLEAR", nReaders,
			cpuTrib.getCurrentCpuStream()->cpuId, this,
			__builtin_return_address(0));
	}
#endif
#endif
#ifdef CONFIG_DEBUG_LOCK_EXCEPTIONS
	cpuTrib.getCurrentCpuStream()->nLocksHeld++;
	cpuTrib.getCurrentCpuStream()->mostRecentlyAcquiredLock = this;
#endif
}

void MultipleReaderLock::readRelease(uarch_t _flags)
{
#if __SCALING__ >= SCALING_SMP
	atomicAsm::decrement(&lock);
#endif
#ifdef CONFIG_DEBUG_LOCK_EXCEPTIONS
	cpuTrib.getCurrentCpuStream()->nLocksHeld--;
#endif

	// Test the flags and see whether or not to enable IRQs.
	if (FLAG_TEST(_flags, Lock::FLAGS_IRQS_WERE_ENABLED)) {
		cpuControl::enableInterrupts();
	};
}

void MultipleReaderLock::writeAcquire(void)
{
#ifdef CONFIG_DEBUG_LOCKS
	// Scale the number of tries based on the number of CPUs
	cpu_t highestCpuId = atomicAsm::read(&CpuStream::highestCpuId);
	uarch_t nReadTriesRemaining = calcDeadlockNReadTries(highestCpuId);
	uarch_t nWriteTriesRemaining = calcDeadlockNWriteTries(highestCpuId);
#endif
	uarch_t contenderFlags = 0;

	if (cpuControl::interruptsEnabled())
	{
		FLAG_SET(contenderFlags, Lock::FLAGS_IRQS_WERE_ENABLED);
		cpuControl::disableInterrupts();
	};

#if __SCALING__ >= SCALING_SMP
	// Spin until we can set the write request bit (it was previously unset)
	while (atomicAsm::bitTestAndSet(&lock, MR_FLAGS_WRITE_REQUEST_SHIFT) != 0)
	{
#ifdef CONFIG_RT_KERNEL_IRQS
		// Re-enable interrupts while we spin if they were enabled before
		if (FLAG_TEST(contenderFlags, Lock::FLAGS_IRQS_WERE_ENABLED))
			{ cpuControl::enableInterrupts(); }
#endif
		cpuControl::subZero();
#ifdef CONFIG_RT_KERNEL_IRQS
		cpuControl::disableInterrupts();
#endif

#ifdef CONFIG_DEBUG_LOCKS
		if (nWriteTriesRemaining-- <= 1) { goto deadlock; };
#endif
	};

	/* Spin on reader count until there are no readers left on the
	 * shared resource.
	 **/
	while ((atomicAsm::read(&lock)
		& ((1 << Lock::FLAGS_ENUM_START) - 1)) != 0)
	{
#ifdef CONFIG_RT_KERNEL_IRQS
		// Re-enable interrupts while we spin if they were enabled before
		if (FLAG_TEST(contenderFlags, Lock::FLAGS_IRQS_WERE_ENABLED))
			{ cpuControl::enableInterrupts(); }
#endif
		cpuControl::subZero();
#ifdef CONFIG_RT_KERNEL_IRQS
		cpuControl::disableInterrupts();
#endif

#ifdef CONFIG_DEBUG_LOCKS
		if (nReadTriesRemaining-- <= 1) { break; };
#endif
	};

#ifdef CONFIG_DEBUG_LOCKS
deadlock:
	if (nReadTriesRemaining <= 1 || nWriteTriesRemaining <= 1)
	{
		uarch_t lockValue = atomicAsm::read(&lock);
		uarch_t nReaders = lockValue & ((1 << MR_FLAGS_WRITE_REQUEST_SHIFT) - 1);
		uarch_t writeRequestSet = (lockValue & MR_FLAGS_WRITE_REQUEST) != 0;

		reportDeadlock(
			FATAL"MultipleReaderLock::writeAcquire[%s] deadlock detected:\n"
			"\tnReadTriesRemaining: %d, nWriteTriesRemaining: %d\n"
			"\tlock addr: %p, lock val: %x\n"
			"\tWrite request bit: %s, Number of readers: %d\n"
			"\tCPU: %d, Lock obj addr: %p, Calling function: %p, "
			"curr ownerAcquisitionInstr: %p",
			name, nReadTriesRemaining, nWriteTriesRemaining, &lock, lock,
			writeRequestSet ? "SET" : "CLEAR", nReaders,
			cpuTrib.getCurrentCpuStream()->cpuId, this,
			__builtin_return_address(0),
			ownerAcquisitionInstr);
	};

	ownerAcquisitionInstr = reinterpret_cast<void(*)()>(
		__builtin_return_address(0));
#endif
#endif
#ifdef CONFIG_DEBUG_LOCK_EXCEPTIONS
	cpuTrib.getCurrentCpuStream()->nLocksHeld++;
	cpuTrib.getCurrentCpuStream()->mostRecentlyAcquiredLock = this;
#endif

	this->flags |= contenderFlags;
}

void MultipleReaderLock::writeRelease(void)
{
	uarch_t		enableIrqs = 0;
	uarch_t		innerWriteRequestBitSet = 0;

	if (FLAG_TEST(flags, Lock::FLAGS_IRQS_WERE_ENABLED))
	{
		FLAG_UNSET(flags, Lock::FLAGS_IRQS_WERE_ENABLED);
		enableIrqs = 1;
	};

#if __SCALING__ >= SCALING_SMP
	/**	EXPLANATION:
	 * There are 2 release cases:
	 *	1. The inner write request bit is set.
	 *	2. The inner write request bit is not set.
	 *
	 * Case 1 means we acquired the lock while we ourselves held the lock
	 * as a reader (i.e: we called readReleaseWriteAcquire()). In this case,
	 * we need to both unset the inner write request bit and decrement the
	 * lock count.
	 *
	 * Case 2 means we acquired the lock trivially and directly via
	 * writeAcquire(). In this case, we just need to unset the write request
	 * bit to release the lock.
	 *
	 *	NB:
	 * The fact that we unset the INNER_WRITE_REQUEST bit before
	 * decrementing the lock count means that we are favouring threads that
	 * already held the lock as a reader. They will have first crack at
	 * acquiring the lock before other threads that are requesting write
	 * access with the plain, outer WRITE_REQUEST bit.
	 **/
	innerWriteRequestBitSet = atomicAsm::bitTestAndClear(
		&lock, MR_FLAGS_INNER_WRITE_REQUEST_SHIFT);
	if (innerWriteRequestBitSet)
		{ atomicAsm::decrement(&lock); }
	else
	{
		uarch_t writeRequestBitSet = atomicAsm::bitTestAndClear(
			&lock, MR_FLAGS_WRITE_REQUEST_SHIFT);
		assert(writeRequestBitSet);
	}
#endif

#ifdef CONFIG_DEBUG_LOCK_EXCEPTIONS
	cpuTrib.getCurrentCpuStream()->nLocksHeld--;
#endif

	if (enableIrqs) {
		cpuControl::enableInterrupts();
	};
}

void MultipleReaderLock::readReleaseWriteAcquire(uarch_t rwFlags)
{
	/* Acquire the lock, set the writer flag, decrement the reader count,
	 * wait for readers to exit. Return.
	 *
	 * We shouldn't enable IRQs between loop iterations of the spin loops
	 * because the caller expects IRQs to remain disabled -- the caller
	 * wishes to maintain her critical section.
	 **/
#if __SCALING__ >= SCALING_SMP
#ifdef CONFIG_DEBUG_LOCKS
	// Scale the number of tries based on the number of CPUs
	cpu_t highestCpuId = atomicAsm::read(&CpuStream::highestCpuId);
	uarch_t nWriteTriesRemaining = calcDeadlockNWriteTries(highestCpuId);
#endif

	// Don't have to CLI cos readAcquire() already CLI'd for us.

	/**	EXPLANATION:
	 * The reader count already has our contribution added to it. This keeps
	 * any outer write requests from being granted while we're still inside
	 * the critical section.
	 *
	 * There may however, be another reader currently inside the critical
	 * section. In order to gain exclusive access with respect to the other
	 * readers, we spin and set the inner write request bit. Once the inner
	 * write request bit is set, we can be sure that no other readers are
	 * inside the critical section.
	 *
	 * At that point, we now have exclusive access with respect to the other
	 * readers and the outer write request.
	 **/
	while (atomicAsm::bitTestAndSet(
		&lock, MR_FLAGS_INNER_WRITE_REQUEST_SHIFT) != 0)
	{
		cpuControl::subZero();
#ifdef CONFIG_DEBUG_LOCKS
		if (nWriteTriesRemaining-- <= 1) { goto deadlock; };
#endif
	}

#ifdef CONFIG_DEBUG_LOCKS
deadlock:
	if (nWriteTriesRemaining <= 1)
	{
		uarch_t lockValue = atomicAsm::read(&lock);
		uarch_t nReaders = lockValue & ((1 << MR_FLAGS_WRITE_REQUEST_SHIFT) - 1);
		uarch_t writeRequestSet = (lockValue & MR_FLAGS_WRITE_REQUEST) != 0;

		reportDeadlock(
			FATAL"MultipleReaderLock::readReleaseWriteAcquire[%s] deadlock detected:\n"
			"\tnWriteTriesRemaining: %d\n"
			"\tlock addr: %p, lock val: %x\n"
			"\tWrite request bit: %s, Number of readers: %d\n"
			"\tCPU: %d, Lock obj addr: %p, Calling function: %p, "
			"curr ownerAcquisitionInstr: %p",
			name, nWriteTriesRemaining, &lock, lock,
			writeRequestSet ? "SET" : "CLEAR", nReaders,
			cpuTrib.getCurrentCpuStream()->cpuId, this,
			__builtin_return_address(0),
			ownerAcquisitionInstr);
	}

	ownerAcquisitionInstr = reinterpret_cast<void(*)()>(
		__builtin_return_address(0));
#endif
#endif

#ifdef CONFIG_DEBUG_LOCK_EXCEPTIONS
	// We don't modify nLocksHeld in readReleaseWriteAcquire().
	cpuTrib.getCurrentCpuStream()->mostRecentlyAcquiredLock = this;
#endif

	flags |= rwFlags;
}

