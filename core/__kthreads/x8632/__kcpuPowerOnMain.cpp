
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

extern "C" void getRegs(registerContextC *t);
//registerContextC		y(0);

void __kcpuPowerOnMain(cpuStreamC *self)
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
	self->baseInit();
	self->powerManager.setPowerStatus(cpuStreamC::powerManagerC::C0);

	// After "bind", the CPU will be able to allocate, etc. normally.
	err = self->bind();
	if (err != ERROR_SUCCESS) {
		__kprintf(FATAL CPUPOWER"%d: Failed to bind().\n", self->cpuId);
	};

	/*__kprintf(NOTICE CPUPOWER"CPU %d: Sleepstack: 0x%x. Regdump:\n"
		"\teax 0x%x, ebx 0x%x, ecx 0x%x, edx 0x%x\n"
		"\tesi 0x%x, edi 0x%x, esp 0x%x, ebp 0x%x\n"
		"\tcs 0x%x, ds 0x%x, es 0x%x, fs 0x%x, gs 0x%x, ss 0x%x\n"
		"\teip 0x%x, eflags 0x%x\n",
		self->cpuId, self->sleepStack,
		y.eax, y.ebx, y.ecx, y.edx, y.esi, y.edi, y.esp, y.ebp,
		y.cs, y.ds, y.es, y.fs, y.gs, y.ss, y.eip, y.eflags);*/

	/* Halt the CPU here. This will be replaced with a call to
	 * taskStreamC::pull() eventually.
	 **/
	self->taskStream.cooperativeBind();
	self->taskStream.pull();
}

