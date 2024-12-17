
#include <scaling.h>
#include <arch/cpuControl.h>
#include <__kstdlib/__kflagManipulation.h>
#include <kernel/common/recursiveLock.h>
#include <kernel/common/thread.h>
#include <kernel/common/processId.h>
#include <kernel/common/cpuTrib/cpuTrib.h>

/**	EXNAPLANATION:
 * On a non-SMP build, there is only ONE processor, so there is no need to
 * spin on a lock. Also, there's no need to check, or even have the 'taskId'
 * member since only one thread can run at once. And that thread will not
 * be able to leave the CPU until it leave the critical section. Therefore,
 * we need not have a count of the number of times it entered the critical
 * section. We can just disable IRQs and increment the 'lock' member on
 * acquire and decrement it on release.
 **/

void RecursiveLock::acquire(void)
{
	Thread		*thread;

	thread = cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentThread();

#if __SCALING__ >= SCALING_SMP

	for (;FOREVER;)
	{
#endif
		taskId.lock.acquire();

#if __SCALING__ >= SCALING_SMP
		if (taskId.rsrc == PROCID_INVALID)
		{
			taskId.rsrc = thread->getFullId();

			/* Check the flags on the waitlock which guards
			 * this critical section to know whether or not
			 * IRQS were enabled before we got to this
			 * lock's acquire.
			 **/
#endif
			if (FLAG_TEST(
				taskId.lock.flags,
				LOCK_FLAGS_IRQS_WERE_ENABLED))
			{
				FLAG_SET(
					flags,
					LOCK_FLAGS_IRQS_WERE_ENABLED);
			};

			// Release the taskId lock as soon as you can.
			taskId.lock.releaseNoIrqs();

			lock++;
#if __SCALING__ >= SCALING_SMP
			return;
		}
		// If the current task already holds the lock:
		else if (taskId.rsrc == thread->getFullId())
		{
			taskId.lock.releaseNoIrqs();
			lock++;
			return;
		}
		else {
			taskId.lock.release();
		};

		// Relax the CPU.
		cpuControl::subZero();
	};
#endif
}

void RecursiveLock::release(void)
{
	/* We make the assumption that a task will only call release() if it
	 * already holds the lock. this is pretty logical. Anyone who does
	 * otherwise is simply being malicious and stupid.
	 *
	 * We can also not acquire the lock on the decrement and the read on
	 * the 'lock' var since only one thread should be able to execute this
	 * code at once. Therefore there is no contention on 'lock' while the
	 * lock is held.
	 **/

#if __SCALING__ >= SCALING_SMP
	uarch_t		enableIrqs=0;
#endif

	lock--;
	// If we're releasing the lock completely, open it up for other tasks.
	if (lock == 0)
	{
		if (FLAG_TEST(flags, LOCK_FLAGS_IRQS_WERE_ENABLED))
		{
			FLAG_UNSET(flags, LOCK_FLAGS_IRQS_WERE_ENABLED);
#if __SCALING__ >= SCALING_SMP
			enableIrqs = 1;
		};
		taskId.lock.acquire();
		taskId.rsrc = PROCID_INVALID;
		taskId.lock.release();
		if (enableIrqs) {
#endif
			cpuControl::enableInterrupts();
		};
	};
}

