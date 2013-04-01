
#include <chipset/zkcm/zkcmCore.h>
#include <arch/cpuControl.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>
#include <kernel/common/interruptTrib/interruptTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


interruptTribC::interruptTribC(void)
{
	for (ubit32 i=0; i<ARCH_INTERRUPTS_NVECTORS; i++)
	{
		msiIrqTable[i].exception = __KNULL;
		msiIrqTable[i].flags = 0;
		msiIrqTable[i].nUnhandled = 0;
		msiIrqTable[i].type = vectorDescriptorS::UNCLAIMED;
	};

	pinIrqTableCounter = 0;
}

error_t interruptTribC::initializeExceptions(void)
{
	/**	EXPLANATION:
	 * Installs the architecture specific exception handlers and initializes
	 * the chipset's IRQ Control module. The IRQ Control mod is initialized
	 * for the /sole/ purpose of being able to call maskAll() and mask off
	 * all IRQs at the chipset's IRQ controller(s).
	 *
	 * The chipset is not expected to register __kpins at this time, and
	 * indeed, if it tries it will most likely fail because dynamic memory
	 * allocation is not yet available.
	 **/
	// Arch specific CPU vector table setup.
	installHardwareVectorTable();

	// Arch specific exception handler setup.
	installExceptions();

	return ERROR_SUCCESS;
}

error_t interruptTribC::initializeIrqManagement(void)
{
	/**	EXPLANATION:
	 * After a call to initialize2(), the chipset is expected to be fully
	 * in synch with the kernel on all information to do with IRQ pins and
	 * bus/device <-> IRQ-pin relationships. The IBM-PC will be used as the
	 * paradigm for explanation:
	 *
	 * On the IBM-PC, this function first sends a notification to the
	 * chipset's IRQ Control module code stating that dynamic memory
	 * allocation is available. Chipsets are expected to respond by
	 * registering all pins on all IRQ controllers with the kernel for
	 * __kpin ID assignment. After this, most chipsets need not do anything
	 * more;
	 *
	 * The IBM-PC is a case where there are so many clone devices from
	 * different vendors that devices are not assigned to specific IRQ pins.
	 * Even the same device being installed on two different boards will be
	 * given two different IRQ pin assignments by the firmware on both
	 * boards. Device <-> IRQ-pin and Bus <-> IRQ-pin mappings vary widely
	 * across boards. On "normal" boards, the hardware features are fixed
	 * for each marketed model, and the wirings for instances of the same
	 * model are the same. IBM-PC compatibles by nature cannot achieve that.
	 *
	 * So for the IBM-PC, simply announcing the available IRQ pins (whether
	 * for the i8259s or for the IO-APICs) is insufficient, because the
	 * IRQ Control module still will not know how PCI devices (or devices
	 * on any bus other than the ISA bus) are mapped to the pins on the IRQ
	 * controllers. Yet, the state of the chipset IRQ Control module after
	 * a call to this function should be that all IRQ pins are registered
	 * with the kernel, _AND_ the IRQ Control module code should also know
	 * the mappings for every device's IRQs, and more specifically, which
	 * pin(s) each device interrupts on.
	 *
	 * So, for the IBM-PC, the notification sent here (dynamic mem alloc
	 * avail) also prompts it to dynamically detect the bus/device <-> pin
	 * mappings on the board.
	 *
	 * The key point is that after this function has been called, the kernel
	 * proper expects that the chipset IRQ Control code should (1) have
	 * registered all of its known IRQ pins with the kernel for __kpin ID
	 * assignment, and (2) the IRQ Control module code should "know" about
	 * all bus/device <-> pin mappings, and be able to translate those on
	 * request from the kernel.
	 **/
	// Notify the IRQ Control mod that memory management is initialized.
	zkcmCore.irqControl.chipsetEventNotification(
		IRQCTL_EVENT_MEMMGT_AVAIL, 0);

	return ERROR_SUCCESS;
}

void interruptTribC::irqMain(taskContextC *regs)
{
	ubit16			__kpin;
	ubit8			triggerMode;
	status_t		status;
	irqPinDescriptorS	*pinDesc;
	isrDescriptorS		*isrDesc;
	void			*handle;
	sarch_t			makeNoise=0;

	if (regs->vectorNo != 253 && regs->vectorNo != 32) { makeNoise = 1; };
	if (makeNoise)
	{
		__kprintf(NOTICE NOLOG INTTRIB"IrqMain: CPU %d entered on "
			"vector %d.\n",
			cpuTrib.getCurrentCpuStream()->cpuId, regs->vectorNo);
	};

	/**	FIXME:
	 * Will have to check for pin IRQs here for now. Pin IRQ vectors should
	 * have their own kernel entry point which doesn't need to examine
	 * vectorDescriptorS at all.
	 **/
	// Ask the chipset if any pin-based IRQs are pending and handle them.
	status = zkcmCore.irqControl.identifyActiveIrq(
		cpuTrib.getCurrentCpuStream()->cpuId,
		regs->vectorNo,
		&__kpin, &triggerMode);

	if (makeNoise)
	{
		__kprintf(NOTICE INTTRIB"irqMain: identifyActiveIrq returned %d, "
			"__kpin %d, triggerMode %d.\n",
			status, __kpin, triggerMode);
	};

	switch (status)
	{
	case IRQCTL_IDENTIFY_ACTIVE_IRQ_SPURIOUS:
		__kprintf(WARNING INTTRIB"irqMain: Spurious IRQ: vector %d.\n",
			regs->vectorNo);

		for (;;) {asm volatile("cli\n\thlt\n\t");};
		return;

	case IRQCTL_IDENTIFY_ACTIVE_IRQ_UNIDENTIFIABLE:
		break;

	default:
		pinDesc = (irqPinDescriptorS *)pinIrqTable.getItem(__kpin);
		if (makeNoise)
		{
			__kprintf(NOTICE INTTRIB"Pin-based IRQ (__kpin %d) on CPU %d.\n"
				"\tDumping: %d unhandled, %d ISRs, %s triggered.\n",
				__kpin, cpuTrib.getCurrentCpuStream()->cpuId,
				pinDesc->nUnhandled, pinDesc->isrList.getNItems(),
				(triggerMode == IRQCTL_IRQPIN_TRIGGMODE_LEVEL)
					? "level" : "edge");
		};

		handle = __KNULL;
		isrDesc = pinDesc->isrList.getNextItem(&handle);
		for (; isrDesc != __KNULL;
			isrDesc = pinDesc->isrList.getNextItem(&handle))
		{
			// For now only ZKCM.
			status = isrDesc->api.zkcm.isr(
				isrDesc->api.zkcm.device, 0);

			if (status != ZKCM_ISR_NOT_MY_IRQ)
			{
				if (triggerMode == IRQCTL_IRQPIN_TRIGGMODE_LEVEL
					&& status == ERROR_SUCCESS)
				{
					isrDesc->nHandled++;
					break;
				};

				// Else error.
				pinDesc->nUnhandled++;
				panic(FATAL"Error handling an IRQ.\n");
			};
		};

		zkcmCore.irqControl.sendEoi(__kpin);
		return;
	};

	switch (msiIrqTable[regs->vectorNo].type)
	{
	case vectorDescriptorS::EXCEPTION:
		(*msiIrqTable[regs->vectorNo].exception)(regs, 0);
		break;

	case vectorDescriptorS::MSI_IRQ:
	case vectorDescriptorS::SWI:
		break;

	case vectorDescriptorS::UNCLAIMED:
		/* This is actually a very serious error. If no message
		 * signaled IRQ has been registered on this vector, and the
		 * vector is not a pin-based IRQ vector, it is still unclaimed
		 * (not an SWI or exception either), then it means that a random
		 * IRQ is coming in on this vector that the kernel has no
		 * control over.
		 *
		 * Maybe later on as the design is refined, we will have a
		 * response to this situation, but for now it mandates a kernel
		 * panic.
		 **/
		__kprintf(FATAL INTTRIB"Entry on UNCLAIMED vector %d from CPU "
			"%d.\n",
			regs->vectorNo, cpuTrib.getCurrentCpuStream()->cpuId);

		panic(ERROR_CRITICAL);

	default:
		break;
	};

	if (makeNoise)
	{
		__kprintf(NOTICE NOLOG INTTRIB"IrqMain: Exiting on CPU %d "
			"vector %d.\n",
			cpuTrib.getCurrentCpuStream()->cpuId, regs->vectorNo);
	};

	// Check to see if the exception requires a "postcall".
	if ((msiIrqTable[regs->vectorNo].type == vectorDescriptorS::EXCEPTION)
		&& __KFLAG_TEST(
			msiIrqTable[regs->vectorNo].flags,
			INTTRIB_EXCEPTION_FLAGS_POSTCALL))
	{
		(msiIrqTable[regs->vectorNo].exception)(regs, 1);
	};
}

void interruptTribC::installException(
	uarch_t vector, __kexceptionFn *exception, uarch_t flags)
{
	if (exception == __KNULL) { return; };
	if (msiIrqTable[vector].type != vectorDescriptorS::UNCLAIMED)
	{
		__kprintf(FATAL INTTRIB"installException: Vector %d is already "
			"occupied.\n",
			vector);

		panic(ERROR_CRITICAL);
		return;
	};

	// Else it's a normal exception installation.
	msiIrqTable[vector].type = vectorDescriptorS::EXCEPTION;
	msiIrqTable[vector].exception = exception;
	msiIrqTable[vector].flags = flags;
}

error_t interruptTribC::zkcmS::registerPinIsr(
	ubit16 __kpin, zkcmDeviceBaseC *dev, zkcmIsrFn *isr, uarch_t /*flags*/
	)
{
	irqPinDescriptorS	*pinDesc;
	isrDescriptorS		*isrDesc;
	error_t			ret;

	if (dev == __KNULL || isr == __KNULL) { return ERROR_INVALID_ARG; };

	pinDesc = (irqPinDescriptorS *)interruptTrib.pinIrqTable
		.getItem(__kpin);

	if (pinDesc == __KNULL)
	{
		__kprintf(ERROR INTTRIB"registerPinIsr: Invalid __kpin %d.\n",
			__kpin);

		return ERROR_INVALID_ARG_VAL;
	};

	// Constructor zeroes it out.
	isrDesc = new isrDescriptorS(interruptTribC::isrDescriptorS::ZKCM);
	if (isrDesc == __KNULL)
	{
		__kprintf(ERROR INTTRIB"registerPinIsr: Failed to alloc "
			"isrDescriptorS object.\n");

		return ERROR_MEMORY_NOMEM;
	};

	isrDesc->api.zkcm.isr = isr;
	isrDesc->api.zkcm.device = dev;

	ret = pinDesc->isrList.insert(isrDesc);
	if (ret != ERROR_SUCCESS)
	{
		__kprintf(ERROR INTTRIB"registerPinIsr: Failed to add new ISR "
			"on __kpin %d.\n\tzkcmDeviceC obj 0x%p, ISR 0x%p.\n",
			__kpin, dev, isr);

		delete isrDesc;
		return ERROR_GENERAL;
	};

	return ERROR_SUCCESS;
}

sarch_t interruptTribC::zkcmS::retirePinIsr(ubit16 __kpin, zkcmIsrFn *isr)
{
	irqPinDescriptorS	*pinDesc;
	isrDescriptorS		*isrDesc;
	void			*handle;

	if (isr == __KNULL) { return 0; };

	pinDesc = (irqPinDescriptorS *)interruptTrib.pinIrqTable
		.getItem(__kpin);

	if (pinDesc == __KNULL)
	{
		__kprintf(ERROR INTTRIB"retirePinIsr: Invalid __kpin %d.\n",
			__kpin);

		return 0;
	};

	handle = __KNULL;
	for (isrDesc = (isrDescriptorS *)pinDesc->isrList.getNextItem(&handle);
		isrDesc != __KNULL;
		isrDesc = (isrDescriptorS *)pinDesc->isrList
			.getNextItem(&handle))
	{
		if (isrDesc->api.zkcm.isr != isr) { continue; };

		// If we found the right one:
		__kprintf(NOTICE INTTRIB"retirePinIsr: Retiring ISR 0x%p on "
			"__kpin %d.\n\t(type: ZKCM), handled %d IRQs.\n",
			isr, __kpin, isrDesc->nHandled);

		pinDesc->isrList.remove(isrDesc);
		delete isrDesc;

		// If there are now no more ISRs on this __kpin, mask it off. 
		if (pinDesc->isrList.getNItems() == 0)
		{
			interruptTrib.__kpinDisable(__kpin);
			// Return 1 if this call caused the pin to be masked.
			return 1;
		};

		return 0;
	};

	// Means this was an attempt to retire an ISR that was never registered.
	return 0;
}

error_t interruptTribC::__kpinEnable(ubit16 __kpin)
{
	irqPinDescriptorS	*pinDesc;

	pinDesc = (irqPinDescriptorS *)pinIrqTable.getItem(__kpin);
	if (pinDesc == __KNULL)
	{
		__kprintf(ERROR INTTRIB"__kpinEnable: Invalid __kpin %d.\n",
			__kpin);

		return ERROR_INVALID_ARG_VAL;
	};

	if (pinDesc->isrList.getNItems() == 0)
	{
		__kprintf(WARNING INTTRIB"__kpinEnable: %d: No ISRs in list.\n",
			__kpin);

		return ERROR_INVALID_OPERATION;
	};

	// Unmask the __kpin.
	zkcmCore.irqControl.unmaskIrq(__kpin);
	return ERROR_SUCCESS;
}

sarch_t interruptTribC::__kpinDisable(ubit16 __kpin)
{
	irqPinDescriptorS	*pinDesc;

	pinDesc = (irqPinDescriptorS *)pinIrqTable.getItem(__kpin);
	if (pinDesc == __KNULL)
	{
		__kprintf(ERROR INTTRIB"__kpinEnable: Invalid __kpin %d.\n",
			__kpin);

		return 0;
	};

	if (pinDesc->isrList.getNItems() != 0)
	{
		__kprintf(WARNING INTTRIB"__kpinEnable: %d: Unable to mask "
			"__kpin: still %d handlers in list.\n",
			__kpin, pinDesc->isrList.getNItems());

		return 0;
	};

	// Mask the __kpin off.
	zkcmCore.irqControl.maskIrq(__kpin);
	return 1;
}

void interruptTribC::removeException(uarch_t vector)
{
	if (msiIrqTable[vector].type != vectorDescriptorS::EXCEPTION)
	{
		__kprintf(ERROR INTTRIB"removeException: Vector %d is not an "
			"exception vector.\n",
			vector);

		return;
	};

	msiIrqTable[vector].exception = __KNULL;
	msiIrqTable[vector].flags = 0;
	msiIrqTable[vector].type = vectorDescriptorS::UNCLAIMED;
}

void interruptTribC::registerIrqPins(ubit16 nPins, zkcmIrqPinS *pinList)
{
	irqPinDescriptorS	*tmp;
	error_t			err;

	for (ubit16 i=0; i<nPins; i++)
	{
		tmp = new irqPinDescriptorS;
		tmp->isrList.initialize();
		if (tmp == __KNULL)
		{
			__kprintf(FATAL INTTRIB"registerIrqPins(): Failed to "
				"alloc mem for pin %d.\n",
				i);

			/* If an IRQ pin cannot be managed due to lack of
			 * memory for metadata, the kernel cannot plausibly
			 * claim to have proper control over the machine from
			 * that point on; an unmanaged pin is nothing but
			 * trouble.
			 **/
			panic(ERROR_MEMORY_NOMEM);
		};

// Removed this bit until further design iterations are completed.
#if 0
		switch (pinList[i].triggerMode)
		{
		case IRQCTL_IRQPIN_TRIGGMODE_LEVEL:
			tmp->triggerMode = INTTRIB_IRQCTL_IRQPIN_TRIGGMODE_LEVEL;
			break;

		case IRQCTL_IRQPIN_TRIGGMODE_EDGE:
			tmp->triggerMode = INTTRIB_IRQCTL_IRQPIN_TRIGGMODE_EDGE;
			break;

		default: break;
		};
#endif

		/**	NOTE: 2012-05-27.
		 * There used to be conditional code here which made use of
		 * hardwareIdListC::findFreeIndex() to re-use indexes in the
		 * __kpin array which were left blank when __kpins were removed
		 * by the chipset code. An example of such an occasion is when
		 * the IBM-PC is switched into multiprocessor mode, and the
		 * i8259 ZKCM code removes the __kpins it registered,
		 * leaving holes in the array.
		 *
		 * This functionality was deliberately removed. The kernel now
		 * guarantees that all pins which are registered in the same
		 * call will be assigned congtiguous __kpin IDs, even at the
		 * expense of leaving indexes unused and consuming memory
		 * unnecessarily.
		 *
		 * The code below however, is highly naive, and it does not try
		 * to optimize for space at all. It simply adds new __kpins
		 * to the end of the array and increments the ID counter. If
		 * someone is interested, they could improve the depth of the
		 * algo below. That said, IRQ pins are not added or removed oft,
		 * and the IBM-PC case is probably one of a very small number
		 * of chipsets whose IRQ pin layout can be dynamically altered
		 * at runtime.
		 **/
		pinList[i].__kid = pinIrqTableCounter;

		// Add it to the list.
		err = pinIrqTable.addItem(pinIrqTableCounter, tmp);
		if (err != ERROR_SUCCESS)
		{
			__kprintf(FATAL INTTRIB"registerPinIrqs(): "
				"Failed to add pin %d to the pin IRQ "
				"table.\n",
				i);

			/* If the kernel cannot monitor an IRQ pin, it cannot
			 * reasonably say that it has control over the chipset.
			 **/
			panic(err);
		};
		pinIrqTableCounter++;
	};
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
	for (ubit16 i=0; i<ARCH_INTERRUPTS_NVECTORS; i++)
	{
		if (msiIrqTable[i].type == vectorDescriptorS::EXCEPTION)
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

static void dumpIsrDescriptor(ubit16 num, interruptTribC::isrDescriptorS *desc)
{
	if (desc->driverType == interruptTribC::isrDescriptorS::ZKCM)
	{
		__kprintf(CC"\t%d (is ZKCM): nHandled %d,"
			"\n\tisr @0x%p, device base @0x%p.\n",
			num, desc->nHandled,
			desc->api.zkcm.isr, desc->api.zkcm.device);
	};
}

void interruptTribC::dumpMsiIrqs(void)
{
	ubit32		nItems;
	isrDescriptorS	*tmp;

	__kprintf(NOTICE INTTRIB"dumpMsiIrqs:\n");

	for (ubit16 i=0; i<ARCH_INTERRUPTS_NVECTORS; i++)
	{
		if (msiIrqTable[i].type == vectorDescriptorS::MSI_IRQ)
		{
			nItems = msiIrqTable[i].isrList.getNItems();
			__kprintf(CC"vec%d: %d handlers.\n", i, nItems);
			for (ubit16 j=0; j<nItems; j++)
			{
				tmp = msiIrqTable[i].isrList.getItem(j);
				dumpIsrDescriptor(j, tmp);
			};
		};
	};
}

void interruptTribC::dumpUnusedVectors(void)
{
	ubit8	flipFlop=0;

	__kprintf(NOTICE INTTRIB"dumpUnusedVectors:\n");
	for (ubit16 i=0; i<ARCH_INTERRUPTS_NVECTORS; i++)
	{
		if (msiIrqTable[i].type == vectorDescriptorS::UNCLAIMED)
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
		__kprintf(CC"\t__kpin %d: %d devices.\n",
			prev, tmp->isrList.getNItems());

		prev = context;
		tmp = reinterpret_cast<irqPinDescriptorS *>(
			pinIrqTable.getLoopItem(&context) );
	};
}

