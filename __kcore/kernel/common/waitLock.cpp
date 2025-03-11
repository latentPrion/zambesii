
#include <config.h>
#include <arch/cpuControl.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/thread.h>
#include <kernel/common/panic.h>
#include <kernel/common/waitLock.h>
#include <kernel/common/sharedResourceGroup.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


#define DEADLOCK_BUFF_MAX_NBYTES	(1024)

struct sDeadlockBuff
{
	sDeadlockBuff(void)
	:
	inUse(0)
	{
		memset(buff, 0, sizeof(buff));
		buffer.rsrc = buff;
	}

	sbit8		inUse;
	utf8Char	buff[DEADLOCK_BUFF_MAX_NBYTES];
	SharedResourceGroup<WaitLock, utf8Char *>	buffer;
} static deadlockBuffers[CONFIG_MAX_NCPUS];

void Lock::sOperationDescriptor::execute()
{
	if (lock == NULL) { panic(FATAL"execute: lock is NULL.\n"); };

	switch (type)
	{
	case WAIT:
		static_cast<WaitLock *>( lock )->release();
		break;

	case RECURSIVE:
		static_cast<RecursiveLock *>( lock )->release();
		break;

	case MULTIPLE_READER:
		switch (operation)
		{
		case READ:
			static_cast<MultipleReaderLock *>( lock )
				->readRelease(rwFlags);

			break;

		case WRITE:
			static_cast<MultipleReaderLock *>( lock )
				->writeRelease();

			break;

		default:
			panic(FATAL"execute: Invalid unlock operation.\n");
		};
		break;

	default: panic(FATAL"execute: Invalid lock type.\n");
	};
}

void WaitLock::acquire(void)
{
	uarch_t	nTries = 10000;
	cpu_t	cid;
	uarch_t contenderFlags=0;

	if (cpuControl::interruptsEnabled())
	{
		FLAG_SET(contenderFlags, LOCK_FLAGS_IRQS_WERE_ENABLED);
		cpuControl::disableInterrupts();
	};

#if __SCALING__ >= SCALING_SMP
	ARCH_ATOMIC_WAITLOCK_HEADER(&lock, 1, 0)
	{
		cpuControl::subZero();
		if (nTries <= 1) { break; };
		nTries--;
	};
#endif

	if (nTries <= 1)
	{
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
			FATAL"Deadlock detected.\n"
			"\tCPU: %d, Lock obj addr: %p. Calling function: %p,\n"
			"\tlock int addr: %p, lockval: %d.\n",
			cid, this, __builtin_return_address(0), &lock, lock);

		cpuControl::disableInterrupts();
		cpuControl::halt();

		/* If we somehow get past the CLI/HLT pair, clear the inUse flag.
		 */
		deadlockBuffers[cid].inUse = 0;
	};

	flags = contenderFlags;
}

void WaitLock::release(void)
{
#if __SCALING__ >= SCALING_SMP
	uarch_t		enableIrqs=0;
#endif

	if (FLAG_TEST(flags, LOCK_FLAGS_IRQS_WERE_ENABLED))
	{
		FLAG_UNSET(flags, LOCK_FLAGS_IRQS_WERE_ENABLED);
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
	FLAG_UNSET(flags, LOCK_FLAGS_IRQS_WERE_ENABLED);
#if __SCALING__ >= SCALING_SMP
	atomicAsm::set(&lock, 0);
#endif
}

