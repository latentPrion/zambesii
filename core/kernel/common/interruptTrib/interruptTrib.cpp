
#include <debug.h>
#include <chipset/zkcm/zkcmCore.h>
#include <asm/cpuControl.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kclib/string.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>
#include <kernel/common/interruptTrib/interruptTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


/**	EXPLANATION:
 * In the event that a chipset has an early device to install an ISR for, it is
 * allowed to install that ISR in the exception slot on the vector its device
 * raises its IRQ on. A device that registers an early ISR uses
 * installException(), passing INTTRIB_VECTOR_FLAGS_BOOTISR.
 *
 * The kernel takes note of the fact that it is an ISR and will ensure that
 * when interruptTribC::initialize2() is called, that ISR is removed from the
 * exception slot and added to the vector's ISR list.
 *
 * CAVEAT: No early ISR may be installed on a vector which has a known actual
 * exception which would be installed by the kernel. For example, on x86-32,
 * that means that a chipset may not install an early ISR on vectors 0-31.
 **/
static ubit8		bootIsrsAllowed=1;

interruptTribC::interruptTribC(void)
{
	for (ubit32 i=0; i<ARCH_IRQ_NVECTORS; i++)
	{
		intTable[i].exception = __KNULL;
		intTable[i].flags = 0;
		intTable[i].nUnhandled = 0;
	};
}

error_t interruptTribC::initialize(void)
{
	/**	EXPLANATION:
	 * Installs the architecture specific exception handlers and initializes
	 * the chipset's intController module.
	 *
	 * The intController initialization also signals to the chipset that
	 * it is safe to set up any early device ISRs, such as a timer IRQ for
	 * the periodic petting of a watchdog device.
	 *
	 * Should the chipset need to do anything of that sort, it should
	 * install a handler for the relevant early device using
	 * installException(v, &handler, INTTRIB_VECTOR_FLAGS_BOOTISR).
	 *
	 * The handler is installed as if it were an exception and not an ISR
	 * until later on when the kernel is able to properly handle ISR
	 * installation. At that point the kernel will internally patch up the
	 * earlier temporary measure. That should suffice for any chipsets which
	 * may require the installation of an ISR for a device earlier than
	 * the kernel's normal IRQ handling logic is initialized.
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
	assert_fatal(zkcmCore.intController != __KNULL);
	assert_fatal((zkcmCore.intController->initialize)() == ERROR_SUCCESS);

	/**	EXPLANATION:
	 * From here on, IRQs will be unmasked one by one as their devices are
	 * discovered and initialized.
	 **/
	// Ask the chipset to mask all IRQs at its irq controller(s).
	(zkcmCore.intController->maskAll)();

	return ERROR_SUCCESS;
}

error_t interruptTribC::initialize2(void)
{
	/**	EXPLANATION:
	 * This is called simply to notify the Interrupt Tributary that memory
	 * management is now initialized fully and the heap is available.
	 *
	 * Now the Interrupt Tributary will cycle through all of the vector
	 * descriptors and find all the (if any) early ISRs installed by the
	 * chipset code and properly add them to their respective vectors' ISR
	 * lists.
	 **/
	for (ubit32 i=0; i<ARCH_IRQ_NVECTORS; i++)
	{
		if (__KFLAG_TEST(
			intTable[i].flags, INTTRIB_VECTOR_FLAGS_BOOTISR))
		{
			/**	TODO:
			 * Move the ISR off the "exception" pointer and properly
			 * add it to the list of ISRs on the vector. Make sure
			 * you add it as a native interface driver and not a UDI
			 * driver.
			 **/
		};
	};

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
	if (regs->vectorNo != 253)
	{
		__kprintf(NOTICE NOLOG INTTRIB"IrqMain: CPU %d entered on "
			"vector %d.\n",
			cpuTrib.getCurrentCpuStream()->cpuId, regs->vectorNo);
	};

	// TODO: Check for a boot ISR.

	// Check for an exception.
	if (__KFLAG_TEST(
		intTable[regs->vectorNo].flags, INTTRIB_VECTOR_FLAGS_EXCEPTION))
	{
		(*intTable[regs->vectorNo].exception)(regs, 0);
	};


	// TODO: Calls ISRs, then exit.

	if (regs->vectorNo != 253)
	{
		__kprintf(NOTICE NOLOG INTTRIB"IrqMain: Exiting on CPU %d "
			"vector %d.\n",
			cpuTrib.getCurrentCpuStream()->cpuId, regs->vectorNo);
	};

	// Check to see if the exception requires a "postcall".
	if (__KFLAG_TEST(
		intTable[regs->vectorNo].flags, INTTRIB_VECTOR_FLAGS_EXCEPTION)
		&& __KFLAG_TEST(
			intTable[regs->vectorNo].flags,
			INTTRIB_VECTOR_FLAGS_EXCEPTION_POSTCALL))
	{
		(intTable[regs->vectorNo].exception)(regs, 1);
	};
}

void interruptTribC::installException(
	uarch_t vector, __kexceptionFn *exception, uarch_t flags)
{
	if (exception == __KNULL) { return; };

	if (__KFLAG_TEST(flags, INTTRIB_VECTOR_FLAGS_SWI))
	{
		__kprintf(WARNING INTTRIB"Failed to install exception 0x%p on "
			"vector %d; is an SWI vector.\n",
			exception, vector);

		return;
	};

	if (__KFLAG_TEST(flags, INTTRIB_VECTOR_FLAGS_BOOTISR))
	{
		if (!bootIsrsAllowed)
		{
			__kprintf(ERROR INTTRIB"Failed to install boot-ISR "
				"0x%p on vector %d: boot ISRs have expired. "
				"\tNormal ISR installation is now initialized."
				"\n", exception, vector);

			return;
		};

		// Chipset wants to install an early ISR.
		__KFLAG_SET(
			intTable[vector].flags, INTTRIB_VECTOR_FLAGS_BOOTISR);
	}
	else
	{
		// Else it's a normal exception installation.
		__KFLAG_SET(
			intTable[vector].flags, INTTRIB_VECTOR_FLAGS_EXCEPTION);
	};

	intTable[vector].exception = exception;
	intTable[vector].flags |= flags;
}

void interruptTribC::removeException(uarch_t vector)
{
	intTable[vector].exception = __KNULL;
	intTable[vector].flags = 0;
}

