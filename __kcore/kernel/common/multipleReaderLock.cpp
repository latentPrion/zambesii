
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

	if (cpuControl::interruptsEnabled()) {
		FLAG_SET(*_flags, Lock::FLAGS_IRQS_WERE_ENABLED);
		cpuControl::disableInterrupts();
	}

#if __SCALING__ >= SCALING_SMP
	uarch_t nReadTriesRemaining = DEADLOCK_READ_MAX_NTRIES;

	atomicAsm::increment(&lock);
	while ((atomicAsm::read(&lock) & MR_FLAGS_WRITE_REQUEST) != 0)
	{
		cpuControl::subZero();
		if (nReadTriesRemaining-- <= 1) { break; };
	}

#ifdef CONFIG_DEBUG_LOCKS
	if (nReadTriesRemaining <= 1) {
		cpu_t cid = cpuTrib.getCurrentCpuStream()->cpuId;
		if (cid == CPUID_INVALID) { cid = 0; }

		if (deadlockBuffers[cid].inUse == 1) {
			panic();
		}

		deadlockBuffers[cid].inUse = 1;
		printf(
			&deadlockBuffers[cid].buffer,
			DEADLOCK_BUFF_MAX_NBYTES,
			FATAL"MRLock::readAcquire deadlock detected: nReadTriesRemaining: %d.\n"
			"\tCPU: %d, Lock obj addr: %p. Calling function: %p,\n"
			"\tlock int addr: %p, lockval: %d.\n",
			nReadTriesRemaining, cid, this, __builtin_return_address(0), &lock, lock);

		deadlockBuffers[cid].inUse = 0;
		cpuControl::halt();
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
	uarch_t nReadTriesRemaining = DEADLOCK_READ_MAX_NTRIES,
		nWriteTriesRemaining = DEADLOCK_WRITE_MAX_NTRIES;
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
		cpuControl::subZero();
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
		cpuControl::subZero();
#ifdef CONFIG_DEBUG_LOCKS
		if (nReadTriesRemaining-- <= 1) { break; };
#endif
	};

#ifdef CONFIG_DEBUG_LOCKS
deadlock:
	if (nReadTriesRemaining <= 1 || nWriteTriesRemaining <= 1)
	{
		cpu_t cid;

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
			FATAL"MRLock::writeAcquire deadlock detected: nReadTriesRemaining: %d, nWriteTriesRemaining: %d.\n"
			"\tCPU: %d, Lock obj addr: %p. Calling function: %p,\n"
			"\tflags addr: %p, flags val: %d.\n",
			nReadTriesRemaining, nWriteTriesRemaining,
			cid, this, __builtin_return_address(0), &flags, flags);

		deadlockBuffers[cid].inUse = 0;
		cpuControl::halt();
	};
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
	 **/
#if __SCALING__ >= SCALING_SMP
	uarch_t nReadTriesRemaining = DEADLOCK_READ_MAX_NTRIES,
		nWriteTriesRemaining = DEADLOCK_WRITE_MAX_NTRIES;

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
		cpu_t cid = cpuTrib.getCurrentCpuStream()->cpuId;
		if (cid == CPUID_INVALID) { cid = 0; };

		if (deadlockBuffers[cid].inUse == 1)
			{ panic(); };

		deadlockBuffers[cid].inUse = 1;

		printf(
			&deadlockBuffers[cid].buffer,
			DEADLOCK_BUFF_MAX_NBYTES,
			FATAL"MRLock::readReleaseWriteAcquire deadlock detected: nReadTriesRemaining: %d, nWriteTriesRemaining: %d.\n"
			"\tCPU: %d, Lock obj addr: %p. Calling function: %p,\n"
			"\tflags addr: %p, flags val: %d.\n",
			nReadTriesRemaining, nWriteTriesRemaining,
			cid, this, __builtin_return_address(0), &flags, flags);

		deadlockBuffers[cid].inUse = 0;
		cpuControl::halt();
	}
#endif
#endif

	flags |= rwFlags;
}

