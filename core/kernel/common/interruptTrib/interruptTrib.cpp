
#include <debug.h>
#include <chipset/pkg/chipsetPackage.h>
#include <asm/cpuControl.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kclib/string.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>
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
	/**	EXPLANATION:
	 * First phase of kernel interrupt reception management: ensure that
	 * the kernel is ready to handle CPU exception interrupts.
	 *
	 * This is mostly a simple architecture specific requirement, and
	 * usually only involves installing pointers to the architecture code's
	 * exception handler functions into the kernel's ISR table.
	 *
	 * Next, the kernel must be able to handle the case of a chipset having
	 * a watchdog device to periodically pet. This may require the chipset
	 * enable local IRQs early on the BSP, and enable a timer device.
	 *
	 * The kernel is not interested in the details for how the per-chipset
	 * code handles any watchdog device it may have; the kernel also for the
	 * most part also assumes that IRQs are still disabled on the BSP, so
	 * the chipset must ensure that any such device is handled quietly.
	 *
	 * Should any form of IRQ handler need to be installed, the chipset
	 * may use the kernel's ISR table and install its timer device handler.
	 * There are two stages in installing support for a watchdog device at
	 * boot. The first is where the chipset code manually dispatches petting
	 * events. Later on in the boot when the kernel has set up a proper
	 * system timer clock source, the chipset should hand over watchdog
	 * management to the kernel.
	 *
	 * (1) Early watchdog handling can be done by the chipset as follows:
	 * register a handler for a timer device, and set up the timer to go off
	 * regularly. The kernel is unaware of this early timer device, so
	 * it should be transparent to the kernel. The handler for the timer
	 * should directly pet the watchdog when the timer IRQ comes in.
	 *
	 * (2) Normal watchdog operation is as follows: the chipset registers a
	 * watchdog ISR with the Timer Tributary. At this point the kernel
	 * should have set up a system timer periodic source. The chipset is to
	 * calculate how many ticks there are between each watchdog pet event,
	 * and tell the kernel. The kernel now takes over petting of the
	 * watchdog device. The chipset is expected to shut down any timer
	 * source associated with the early watchdog handling in (1) above.
	 *
	 * The chipset may choose to only use (2), in which case it is expected
	 * that the watchdog will not reset the system before the kernel is able
	 * to get to the point where it can handle the normal case. Else, the
	 * chipset may use (1) until the kernel is able to handle the normal
	 * case, and then switch over to (2).
	 **/
	// Arch specific CPU vector table setup.
	installHardwareVectorTable();

	// Arch specific exception handler setup.
	installExceptions();

	/* Chipset specific call in to notify the chipset that it may begin
	 * any necessary watchdog device setup, or early IRQ management setup.
	 * The kernel expects IRQs to be masked off at the BSP until
	 * interruptTribC::initialize2() is called.
	 *
	 * So if the chipset does need to install support for a watchdog device,
	 * for example, it is expected to do so in a manner that is transparent
	 * to the kernel, until the kernel later notifies the chipset that it is
	 * about to enable interrupts on the BSP. The kernel is not at
	 * all prepared to handle device IRQs right at this point, mainly
	 * because this function, interruptTribC::initialize() is called before
	 * memory management is initialized.
	 **/
	// Check for int controller, halt if none, else call initialize().
	assert_fatal(chipsetPkg.intController != __KNULL);
	assert_fatal(
		(*chipsetPkg.intController->initialize)() == ERROR_SUCCESS);

	/**	EXPLANATION:
	 * From here on, IRQs will be unmasked one by one as their devices are
	 * discovered and initialized.
	 **/
	// Ask the chipset to mask all IRQs at its irq controller(s).
	(*chipsetPkg.intController->maskAll)();

	return ERROR_SUCCESS;
}

static void *getCr2(void)
{
	void *ret;
	asm volatile (
		"movl	%%cr2, %0\n\t"
		: "=r" (ret));

	return ret;
}



void interruptTribC::irqMain(taskContextS *regs)
{
	__kprintf(NOTICE NOLOG"interruptTribC::irqMain: CPU %d entered "
		"on vector %d.\n",
		cpuTrib.getCurrentCpuStream()->cpuId, regs->vectorNo);

	if (__KFLAG_TEST(
		isrTable[regs->vectorNo].flags,
		INTERRUPTTRIB_VECTOR_FLAGS_EXCEPTION))
	{
		(*isrTable[regs->vectorNo].exception)(regs);
	};

	__kprintf(NOTICE NOLOG"interruptTribC::irqMain: Exiting on CPU %d.\n",
		cpuTrib.getCurrentCpuStream()->cpuId);

	// Calls ISRs, then exit.
}

void interruptTribC::installException(uarch_t vector, exceptionFn *exception)
{
	isrTable[vector].exception = exception;
	__KFLAG_SET(
		isrTable[vector].flags, INTERRUPTTRIB_VECTOR_FLAGS_EXCEPTION);
}

void interruptTribC::removeException(uarch_t vector)
{
	isrTable[vector].exception = __KNULL;
	__KFLAG_SET(
		isrTable[vector].flags, INTERRUPTTRIB_VECTOR_FLAGS_EXCEPTION);
}

