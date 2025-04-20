
#include <debug.h>
#include <arch/paging.h>
#include <arch/registerContext.h>
#include <arch/cpuControl.h>
#include <__kstdlib/__ktypes.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <__kthreads/__kcpuPowerOn.h>


#define CPUPOWER		"CPU Poweron: "


uarch_t getEip(void) __attribute__((noinline));
uarch_t getEip(void)
{
	volatile uarch_t		eip=0;

	/* Stack should look like this:
	 *	eip(12) -> ebp(8) -> | eip(4) | eax
	 */
	asm volatile (
		"pushl	%%eax\n\t \
		movl	12(%%esp), %%eax\n\t \
		movl	%%eax, 4(%%esp)\n\t \
		popl	%%eax\n\t"
		:::"memory");

	return eip;
}

extern "C" void getRegs(RegisterContext *t);
RegisterContext		y(0);

void __kcpuPowerOnMain(CpuStream *cs)
{
	error_t		err;

	/**	EXPLANATION:
	 * Main path for a waking x86-32 CPU. Do the basics to get the CPU in
	 * synch with the kernel.
	 *
	 * Waking CPUs MUST NOT allocate memory from the kernel address space
	 * or do anything which would require accessing or changing the kernel
	 * page tables and translation information. Right now, other CPUs are
	 * waking up, and when a kernel page is mapped anew, we need to
	 * flush the TLB on all other CPUs.
	 *
	 * But since we're currently waking APs, we could send out a global
	 * TLB flush request before a CPU is fully initialized and in a state
	 * to handle inter-CPU-messages, and thus cause that waking CPU to
	 * crash.
	 **/
	cs->initializeBaseState();
	cs->initializeExceptions();
	cs->powerManager.setPowerStatus(CpuStream::PowerManager::C0);

	// After "bind", the CPU will be able to allocate, etc. normally.
	err = cs->bind();
	if (err != ERROR_SUCCESS) {
		printf(FATAL CPUPOWER"%d: Failed to bind().\n", cs->cpuId);
	};

/*	getRegs(&y);
	printf(NOTICE CPUPOWER"CPU %d @%p: Sleepstack: %x. Regdump:\n"
		"\teax %x, ebx %x, ecx %x, edx %x\n"
		"\tesi %x, edi %x, esp %x, ebp %x\n"
		"\tcs %x, ds %x, es %x, fs %x, gs %x, ss %x\n"
		"\teip %x, eflags %x\n",
		self->cpuId, self, self->schedStack,
		y.eax, y.ebx, y.ecx, y.edx, y.esi, y.edi, y.esp, y.ebp,
		y.cs, y.ds, y.es, y.fs, y.gs, y.ss, y.eip, y.eflags);
*/

	/* Halt the CPU here. This will be replaced with a call to
	 * TaskStream::pull() eventually.
	 **/
	cs->taskStream.cooperativeBind();
	printf(NOTICE CPUPOWER"%d: PowerOnMain: done. About to taskStream.pull()\n", cs->cpuId);

	Thread		*self = cs->taskStream.getCurrentThread();
	if (self == NULL)
	{
		printf(FATAL CPUPOWER"%d: Failed to get current thread from "
			"TaskStream.\n", cs->cpuId);

		cpuControl::disableInterrupts();
		cpuControl::halt();
	}

	MessageStream::sHeader		*iMsg;
	for (;FOREVER;)
	{
		err = self->messageStream.pull(&iMsg);
		if (err != ERROR_SUCCESS)
		{
			printf(FATAL CPUPOWER"%d: Failed to pull message from "
				"MessageStream.\n", cs->cpuId);

			continue;
		}

		printf(NOTICE CPUPOWER"%d: Pulled message from MessageStream: "
			"Subsystem %d, function %d.\n",
			cs->cpuId, iMsg->subsystem, iMsg->function);

//		delete iMsg;
	}
	
}

