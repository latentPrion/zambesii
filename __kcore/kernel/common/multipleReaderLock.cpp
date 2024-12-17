
#include <arch/atomic.h>
#include <arch/cpuControl.h>
#include <__kstdlib/__kflagManipulation.h>
#include <kernel/common/multipleReaderLock.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


/**	FIXME:
 * A series of race conditions exist in this class, mostly with the writer
 * acquire/release functions and the readReleaseWriteAcquire function.
 *
 * Get rid of these whenever next there is a freeze period.
 **/
void MultipleReaderLock::readAcquire(uarch_t *_flags)
{
	readerCount.lock.acquire();

	// Save the thread's current IRQ state.
	*_flags = readerCount.lock.flags;
#if __SCALING__ >= SCALING_SMP
	readerCount.rsrc++;
#endif

	readerCount.lock.releaseNoIrqs();
}

void MultipleReaderLock::readRelease(uarch_t _flags)
{
#if __SCALING__ >= SCALING_SMP
	// See if there is a writer waiting on the lock.
	if (!FLAG_TEST(
		atomicAsm::read(&flags), MRLOCK_FLAGS_WRITE_REQUEST))
	{
		/* If there is no writer waiting, acquire the lock and then
		 * decrement. Else just decrement.
		 **/
		readerCount.lock.acquire();
		readerCount.rsrc--;
		readerCount.lock.releaseNoIrqs();
	}
	else
	{
		// If there is a writer, don't acquire the lock. Just decrement.
		atomicAsm::decrement(&readerCount.rsrc);
	};
#endif

	// Test the flags and see whether or not to enable IRQs.
	if (FLAG_TEST(_flags, LOCK_FLAGS_IRQS_WERE_ENABLED)) {
		cpuControl::enableInterrupts();
	};
}

void MultipleReaderLock::writeAcquire(void)
{
	// Acquire the readerCount lock.
	readerCount.lock.acquire();

#if __SCALING__ >= SCALING_SMP
	FLAG_SET(flags, MRLOCK_FLAGS_WRITE_REQUEST);

	/* Spin on reader count until there are no readers left on the
	 * shared resource.
	 **/
	while (atomicAsm::read(&readerCount.rsrc) > 0) {
		cpuControl::subZero();
	};

	/* Since we don't release the lock before returning, the waitLock holds
	 * IRQ state for us already.
	 **/
#endif
}

void MultipleReaderLock::writeRelease(void)
{
	/* Do not attempt to acquire the lock. We already hold it. Just unset
	 * the write request flag and release the lock.
	 **/
#if __SCALING__ >= SCALING_SMP
	FLAG_UNSET(flags, MRLOCK_FLAGS_WRITE_REQUEST);
#endif

	readerCount.lock.release();
}

void MultipleReaderLock::readReleaseWriteAcquire(uarch_t rwFlags)
{
	/* Acquire the lock, set the writer flag, decrement the reader count,
	 * wait for readers to exit. Return.
	 **/
	readerCount.lock.acquire();

#if __SCALING__ >= SCALING_SMP
	FLAG_SET(flags, MRLOCK_FLAGS_WRITE_REQUEST);
	// Decrement reader count.
	readerCount.rsrc--;

	// Spin on reader count until no readers are left.
	while (atomicAsm::read(&readerCount.rsrc) > 0) {
		cpuControl::subZero();
	};
#endif

	/* It doesn't matter whether or not IRQs were enabled before the
	 * readAcquire() that preceded the call to this function. The write
	 * acquire must have IRQs disabled. So just save IRQ state in the
	 * readerCount lock.
	 **/
	readerCount.lock.flags |= rwFlags;
}

