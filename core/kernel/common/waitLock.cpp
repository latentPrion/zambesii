
#include <arch/cpuControl.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/task.h>
#include <kernel/common/panic.h>
#include <kernel/common/waitLock.h>
#include <kernel/common/sharedResourceGroup.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


static sharedResourceGroupC<waitLockC, void *>	buffDescriptors[16];
static utf8Char		buffers[16][1024];

void waitLockC::acquire(void)
{
	uarch_t	nTries = 0xF00000;
	cpu_t	cid;
	taskC	*task;

	if (cpuControl::interruptsEnabled())
	{
		__KFLAG_SET(lockC::flags, LOCK_FLAGS_IRQS_WERE_ENABLED);
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
		buffDescriptors[cid].rsrc =
			static_cast<void *>( &buffers[cid][0] );

		__kprintf(&buffDescriptors[cid], 8192,
			FATAL"Deadlock detected.\n"
			"\tCPU: %d. Lock address: 0x%p. Calling function: 0x%p, lockval: %d.\n",
			cid, this, __builtin_return_address(0), lock);

		asm volatile("cli\n\thlt\n\t");
	};

	task = cpuTrib.getCurrentCpuStream()->taskStream.currentTask;
	/* On a non-SMP build, this just indicates the number of critical
	 * sections deep into the kernel this thread has currently traveled.
	 **/
	task->nLocksHeld++;
}

void waitLockC::release(void)
{
	taskC	*task;
#if __SCALING__ >= SCALING_SMP
	uarch_t		enableIrqs=0;
#endif

	if (__KFLAG_TEST(flags, LOCK_FLAGS_IRQS_WERE_ENABLED))
	{
		__KFLAG_UNSET(flags, LOCK_FLAGS_IRQS_WERE_ENABLED);
#if __SCALING__ >= SCALING_SMP
		enableIrqs = 1;
	};

	atomicAsm::set(&lock, 0);

	if (enableIrqs) {
#endif
		cpuControl::enableInterrupts();
	};

	task = cpuTrib.getCurrentCpuStream()->taskStream.currentTask;
	task->nLocksHeld--;

}

void waitLockC::releaseNoIrqs(void)
{
	taskC	*task;

	__KFLAG_UNSET(flags, LOCK_FLAGS_IRQS_WERE_ENABLED);
#if __SCALING__ >= SCALING_SMP
	atomicAsm::set(&lock, 0);
#endif

	task = cpuTrib.getCurrentCpuStream()->taskStream.currentTask;
	task->nLocksHeld--;
}

