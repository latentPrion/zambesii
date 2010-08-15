
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kclib/string.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>
#include <kernel/common/moduleApis/chipsetSupportPackage.h>
#include <kernel/common/interruptTrib/interruptTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>

interruptTribC::interruptTribC(void)
{
	memset(
		isrTable,
		0,
		sizeof(interruptTribC::vectorDescriptorS) * ARCH_IRQ_NVECTORS);
}

error_t interruptTribC::initialize(void)
{
	installHardwareVectorTable();

	/**	EXPLANATION:
	 * At initialization of the Interrupt Tributary, we wish to ensure that
	 * all vectors are masked at the interrupt controller immediately after
	 * loading the hardware vector table into the processor.
	 *
	 * But making a single call to
	 * (*chipsetCoreDev.intController->maskAll)() is not intelligent at all:
	 * we have previously called to the chipset and initialized the
	 * watchdog timer if it exists. In the event that the chipset *does*
	 * have a watchdog, the chipset support code would also have registered
	 * a timer device with the kernel to interrupt at regular intervals and
	 * call the watchdog.
	 *
	 * See, if we call maskAll at the interrupt controller now, we'll mask
	 * off the continuous timer, and hence leave the watchdog unfed. So,
	 * while it would be much faster to just call maskAll, we have to go
	 * one by one, and make sure first that there are no registered ISRs
	 * in the ISR list, and for each unfilled ISR slot, we call
	 * maskSingle() on that slot.
	 *
	 * This is not as bad as it seems for two reasons:
	 *	1. On most processors there aren't a lot of interrupt vectors.
	 *	   x86 is a nice exception in that sense.
	 *	2. Since we loop and mask each interrupt one by one, we get to
	 *	   immediately see, at boot, which interrupts cannot be masked
	 *	   by the chipset support code that was compiled in with our
	 *	   kernel build.
	 **/
	// Check for int controller, halt if none, else call initialize().
	assert_fatal(chipsetCoreDev.intController != __KNULL);
	assert_fatal(
		(*chipsetCoreDev.intController->initialize)() == ERROR_SUCCESS);

	// Mask every interrupt that has no handler currently installed.
	for (uarch_t i=0; i<ARCH_IRQ_NVECTORS; i++)
	{
		if (!__KFLAG_TEST(
			isrTable[i].flags, INTERRUPTTRIB_VECTOR_FLAGS_BOOTISR))
		{
			if ((*chipsetCoreDev.intController->maskSingle)(i)
				!= ERROR_SUCCESS)
			{
				__KFLAG_SET(
					isrTable[i].flags,
					INTERRUPTTRIB_VECTOR_FLAGS_MASKFAILS);
			}
			else
			{
				__KFLAG_SET(
					isrTable[i].flags,
					INTERRUPTTRIB_VECTOR_FLAGS_MASKED);
			};
		};
	};

	/* At this point all unneeded vectors are masked off, and their
	 * relevant flags are set to show that. All we need to do now is install
	 * the architecture specific exception handlers into the ISR table.
	 **/
	installExceptions();
	return ERROR_SUCCESS;
}

void interruptTribC::irqMain(taskContextS *regs)
{
	__kprintf(NOTICE"Interrupt Trib: CPU %d: Entry from vector %d.\n",
		cpuTrib.getCurrentCpuStream()->cpuId, regs->vectorNo);

	if (isrTable[regs->vectorNo].handler.isr) {
		isrTable[regs->vectorNo].handler.except(regs);
	};
	// Calls ISRs, then exit.
}

