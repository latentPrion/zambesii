
#include <debug.h>
#include <chipset/zkcm/zkcmCore.h>
#include <asm/cpuControl.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/new>
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
		msiIrqTable[i].exception = __KNULL;
		msiIrqTable[i].flags = 0;
		msiIrqTable[i].nUnhandled = 0;
	};

	pinIrqTableCounter = 0;
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
	assert_fatal(zkcmCore.irqControl != __KNULL);
	assert_fatal((zkcmCore.irqControl->initialize)() == ERROR_SUCCESS);

	/**	EXPLANATION:
	 * From here on, IRQs will be unmasked one by one as their devices are
	 * discovered and initialized.
	 **/
	// Ask the chipset to mask all IRQs at its irq controller(s).
	(zkcmCore.irqControl->maskAll)();
	return ERROR_SUCCESS;
}

error_t interruptTribC::initialize2(void)
{
	zkcmIrqPinS		*chipsetPins;
	ubit16			nPins;
	error_t			ret;

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
			msiIrqTable[i].flags, INTTRIB_VECTOR_FLAGS_BOOTISR))
		{
			/**	TODO:
			 * Move the ISR off the "exception" pointer and properly
			 * add it to the list of ISRs on the vector. Make sure
			 * you add it as a native interface driver and not a UDI
			 * driver.
			 **/
		};
	};

	/**	EXPLANATION:
	 * Set up pin-based IRQ management:
	 * Ask the chipset how many IRQ pins exist, and build the kernel's
	 * pinIrqTable.
	 **/
	ret = (*zkcmCore.irqControl->getInitialPinInfo)(&nPins, &chipsetPins);
	if (ret != ERROR_SUCCESS)
	{
		panic(ret, CC INTTRIB"initialize2(): Chipset returned an error "
			"when asked about PIC pin info.\n"
			"Kernel cannot continue without IRQ management.\n");
	};

	registerIrqPins(nPins, chipsetPins);
	(*zkcmCore.irqControl->__kregisterPinIds)(nPins, chipsetPins);
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
	if (regs->vectorNo /*!= 253*/)
	{
		__kprintf(NOTICE NOLOG INTTRIB"IrqMain: CPU %d entered on "
			"vector %d.\n",
			cpuTrib.getCurrentCpuStream()->cpuId, regs->vectorNo);
	};

	// TODO: Check for a boot ISR.

	// Check for an exception.
	if (__KFLAG_TEST(
		msiIrqTable[regs->vectorNo].flags, INTTRIB_VECTOR_FLAGS_EXCEPTION))
	{
		(*msiIrqTable[regs->vectorNo].exception)(regs, 0);
	};


	// TODO: Calls ISRs, then exit.

	if (regs->vectorNo /*!= 253*/)
	{
		__kprintf(NOTICE NOLOG INTTRIB"IrqMain: Exiting on CPU %d "
			"vector %d.\n",
			cpuTrib.getCurrentCpuStream()->cpuId, regs->vectorNo);
	};

	// Check to see if the exception requires a "postcall".
	if (__KFLAG_TEST(
		msiIrqTable[regs->vectorNo].flags, INTTRIB_VECTOR_FLAGS_EXCEPTION)
		&& __KFLAG_TEST(
			msiIrqTable[regs->vectorNo].flags,
			INTTRIB_VECTOR_FLAGS_EXCEPTION_POSTCALL))
	{
		(msiIrqTable[regs->vectorNo].exception)(regs, 1);
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
			msiIrqTable[vector].flags, INTTRIB_VECTOR_FLAGS_BOOTISR);
	}
	else
	{
		// Else it's a normal exception installation.
		__KFLAG_SET(
			msiIrqTable[vector].flags, INTTRIB_VECTOR_FLAGS_EXCEPTION);
	};

	msiIrqTable[vector].exception = exception;
	msiIrqTable[vector].flags |= flags;
}

void interruptTribC::removeException(uarch_t vector)
{
	msiIrqTable[vector].exception = __KNULL;
	msiIrqTable[vector].flags = 0;
}

void interruptTribC::registerIrqPins(ubit16 nPins, zkcmIrqPinS *pinList)
{
	irqPinDescriptorS	*tmp;
	error_t			err;

	for (ubit16 i=0; i<nPins; i++)
	{
		tmp = new irqPinDescriptorS;
		if (tmp == __KNULL)
		{
			__kprintf(FATAL INTTRIB"registerIrqPins(): Failed to "
				"alloc mem for pin %d.\n",
				i);

			panic(ERROR_MEMORY_NOMEM);
		};

		switch (pinList[i].triggerMode)
		{
		case IRQPIN_TRIGGMODE_LEVEL:
			tmp->triggerMode = INTTRIB_IRQPIN_TRIGGMODE_LEVEL;
			break;

		case IRQPIN_TRIGGMODE_EDGE:
			tmp->triggerMode = INTTRIB_IRQPIN_TRIGGMODE_EDGE;
			break;

		default: break;
		};

		/* Assign the pin a kernel ID. IDs are taken from the current
		 * value of the counter.
		 **/
		pinList[i].__kid = pinIrqTableCounter;

		// Add it to the list.
		err = pinIrqTable.addItem(pinIrqTableCounter, tmp);
		if (err != ERROR_SUCCESS)
		{
			__kprintf(FATAL INTTRIB"registerPinIrqs(): Failed to "
				"add pin %d to the pin IRQ table.\n",
				i);

			panic(err);
		};
		pinIrqTableCounter++;
	};

	// Return the list with our global kernel pin IDs.
	(*zkcmCore.irqControl->__kregisterPinIds)(nPins, pinList);
}

void interruptTribC::removeIrqPins(ubit16 nPins, zkcmIrqPinS *pinList)
{
	irqPinDescriptorS		*tmp;

	for (ubit16 i=0; i<nPins; i++)
	{
		tmp = reinterpret_cast<irqPinDescriptorS *>(
			pinIrqTable.getItem(pinList[i].__kid) );

		// TODO: Remove each handler registered on the pin as well.
		pinIrqTable.removeItem(pinList[i].__kid);
		delete tmp;
	};
}

void interruptTribC::dumpExceptions(void)
{
	ubit8		flipFlop=0;

	__kprintf(NOTICE INTTRIB"dumpExceptions:\n");
	for (ubit16 i=0; i<ARCH_IRQ_NVECTORS; i++)
	{
		if (__KFLAG_TEST(
			msiIrqTable[i].flags, INTTRIB_VECTOR_FLAGS_EXCEPTION))
		{
			__kprintf(CC"\t%d: 0x%p,", i, msiIrqTable[i].exception);
			if (flipFlop == 3)
			{
				__kprintf(CC"\n");
				flipFlop = 0;
			}
			else { flipFlop++; };
		};
	};
	if (flipFlop != 0) { __kprintf(CC"\n"); };
}

void interruptTribC::dumpMsiIrqs(void)
{
	ubit32		nItems;
	isrDescriptorS	*tmp;

	__kprintf(NOTICE INTTRIB"dumpMsiIrqs:\n");

	for (ubit16 i=0; i<ARCH_IRQ_NVECTORS; i++)
	{
		if (msiIrqTable[i].isrList.getNItems() > 0)
		{
			nItems = msiIrqTable[i].isrList.getNItems();
			__kprintf(CC"vec%d: %d handlers.\n", i, nItems);
			for (ubit16 j=0; j<nItems; j++)
			{
				tmp = msiIrqTable[i].isrList.getItem(j);
				__kprintf(CC"\t%d: %s; handled %d, "
					"procId 0x%x, ISR at 0x%p.\n",
					j,
					((tmp->driverType
						== INTTRIB_ISR_DRIVERTYPE_ZKCM)
						? "ZKCM" : "UDI"),
					tmp->nHandled, tmp->processId,
					tmp->isr);
			};
		};
	};
}

void interruptTribC::dumpUnusedVectors(void)
{
	ubit8	flipFlop=0;

	__kprintf(NOTICE INTTRIB"dumpUnusedVectors:\n");
	for (ubit16 i=0; i<ARCH_IRQ_NVECTORS; i++)
	{
		if (!__KFLAG_TEST(
			msiIrqTable[i].flags, INTTRIB_VECTOR_FLAGS_EXCEPTION)
			&& msiIrqTable[i].isrList.getNItems() == 0)
		{
			__kprintf(CC"\t%d,", i);
			if (flipFlop == 7)
			{
				flipFlop = 0;
				__kprintf(CC"\n");
			}
			else { flipFlop++; };
		};
	};
	if (flipFlop != 0) { __kprintf(CC"\n"); };
}

void interruptTribC::dumpIrqPins(void)
{
	sarch_t			context, prev;
	irqPinDescriptorS	*tmp;

	__kprintf(NOTICE INTTRIB"dumpIrqPins:\n");

	prev = context = pinIrqTable.prepareForLoop();
	tmp = reinterpret_cast<irqPinDescriptorS *>(
		pinIrqTable.getLoopItem(&context) );

	while (tmp != __KNULL)
	{
		__kprintf(CC"\t__kpin %d: Trigger mode %s, %d devices.\n",
			prev,
			((tmp->triggerMode == INTTRIB_IRQPIN_TRIGGMODE_LEVEL) ?
				"level" : "edge"),
			tmp->irqList.getNItems());

		prev = context;
		tmp = reinterpret_cast<irqPinDescriptorS *>(
			pinIrqTable.getLoopItem(&context) );
	};
}

