
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

	atomicAsm::increment(&lock);
	while ((atomicAsm::read(&lock) & MR_FLAGS_WRITE_REQUEST) != 0)
	{
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
		if (nReadTriesRemaining-- <= 1) { break; };
#endif
	}

#ifdef CONFIG_DEBUG_LOCKS
	if (nReadTriesRemaining <= 1)
	{
		uarch_t lockValue = atomicAsm::read(&lock);
		uarch_t nReaders = lockValue & ((1 << MR_FLAGS_WRITE_REQUEST_SHIFT) - 1);
		uarch_t writeRequestSet = (lockValue & MR_FLAGS_WRITE_REQUEST) != 0;

		reportDeadlock(
			FATAL"MultipleReaderLock::readAcquire deadlock detected:\n"
			"\tnReadTriesRemaining: %d, lock int addr: %p, lockval: %x\n"
			"\tWrite request bit: %s, Number of readers: %d\n"
			"\tCPU: %d, Lock obj addr: %p, Calling function: %p",
			nReadTriesRemaining, &lock, lock,
			writeRequestSet ? "SET" : "CLEAR", nReaders,
			cpuTrib.getCurrentCpuStream()->cpuId, this,
			__builtin_return_address(0));
	}
#endif
#endif
}

void MultipleReaderLock::readRelease(uarch_t _flags)
{
#if __SCALING__ >= SCALING_SMP
	atomicAsm::decrement(&lock);
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
			FATAL"MultipleReaderLock::writeAcquire deadlock detected:\n"
			"\tnReadTriesRemaining: %d, nWriteTriesRemaining: %d\n"
			"\tlock addr: %p, lock val: %x\n"
			"\tWrite request bit: %s, Number of readers: %d\n"
			"\tCPU: %d, Lock obj addr: %p, Calling function: %p, "
			"curr ownerAcquisitionInstr: %p",
			nReadTriesRemaining, nWriteTriesRemaining, &lock, lock,
			writeRequestSet ? "SET" : "CLEAR", nReaders,
			cpuTrib.getCurrentCpuStream()->cpuId, this,
			__builtin_return_address(0),
			ownerAcquisitionInstr);
	};

	ownerAcquisitionInstr = reinterpret_cast<void(*)()>(
		__builtin_return_address(0));
#endif
#endif

	this->flags |= contenderFlags;
}

void MultipleReaderLock::writeRelease(void)
{
	uarch_t enableIrqs = 0;

	if (FLAG_TEST(flags, Lock::FLAGS_IRQS_WERE_ENABLED))
	{
		FLAG_UNSET(flags, Lock::FLAGS_IRQS_WERE_ENABLED);
		enableIrqs = 1;
	};

#if __SCALING__ >= SCALING_SMP
	// Clear the write request bit to release the lock
	atomicAsm::bitTestAndClear(&lock, MR_FLAGS_WRITE_REQUEST_SHIFT);
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
	uarch_t nReadTriesRemaining = calcDeadlockNReadTries(highestCpuId);
	uarch_t nWriteTriesRemaining = calcDeadlockNWriteTries(highestCpuId);
#endif

	// Don't have to CLI cos readAcquire() already CLI'd for us.
	while (atomicAsm::bitTestAndSet(&lock, MR_FLAGS_WRITE_REQUEST_SHIFT) != 0)
	{
		cpuControl::subZero();
#ifdef CONFIG_DEBUG_LOCKS
		if (nWriteTriesRemaining-- <= 1) { goto deadlock; };
#endif
	}

	// Decrement our contribution to the reader count.
	atomicAsm::decrement(&lock);

	// Wait for the other readers to exit.
	while ((atomicAsm::read(&lock)
		& ((1 << Lock::FLAGS_ENUM_START) - 1)) != 0)
	{
		cpuControl::subZero();
#ifdef CONFIG_DEBUG_LOCKS
		if (nReadTriesRemaining-- <= 1) { break; };
#endif
	}

#ifdef CONFIG_DEBUG_LOCKS
deadlock:
	if (nReadTriesRemaining <= 1 || nWriteTriesRemaining <= 1)
	{
		uarch_t lockValue = atomicAsm::read(&lock);
		uarch_t nReaders = lockValue & ((1 << MR_FLAGS_WRITE_REQUEST_SHIFT) - 1);
		uarch_t writeRequestSet = (lockValue & MR_FLAGS_WRITE_REQUEST) != 0;

		reportDeadlock(
			FATAL"MultipleReaderLock::readReleaseWriteAcquire deadlock detected:\n"
			"\tnReadTriesRemaining: %d, nWriteTriesRemaining: %d\n"
			"\tlock addr: %p, lock val: %x\n"
			"\tWrite request bit: %s, Number of readers: %d\n"
			"\tCPU: %d, Lock obj addr: %p, Calling function: %p, "
			"curr ownerAcquisitionInstr: %p",
			nReadTriesRemaining, nWriteTriesRemaining, &lock, lock,
			writeRequestSet ? "SET" : "CLEAR", nReaders,
			cpuTrib.getCurrentCpuStream()->cpuId, this,
			__builtin_return_address(0),
			ownerAcquisitionInstr);
	}

	ownerAcquisitionInstr = reinterpret_cast<void(*)()>(
		__builtin_return_address(0));
#endif
#endif

	flags |= rwFlags;
}

