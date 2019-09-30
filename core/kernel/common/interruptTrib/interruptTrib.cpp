
#include <debug.h>
#include <chipset/zkcm/zkcmCore.h>
#include <arch/cpuControl.h>
#include <arch/atomic.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <__kclasses/heapList.h>
#include <kernel/common/panic.h>
#include <kernel/common/interruptTrib/interruptTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


InterruptTrib::InterruptTrib(void)
:
pinIrqTableCounter(0)
{
	for (ubit32 i=0; i<ARCH_INTERRUPTS_NVECTORS; i++)
	{
		msiIrqTable[i].exception = NULL;
		msiIrqTable[i].flags = 0;
		msiIrqTable[i].nUnhandled = 0;
		msiIrqTable[i].type = sVectorDescriptor::UNCLAIMED;
	};
}

error_t InterruptTrib::initializeExceptions(void)
{
	/**	EXPLANATION:
	 * Installs the architecture specific exception handlers and initializes
	 * the chipset's IRQ Control module.
	 *
	 * The chipset is not expected to register __kpins at this time, and
	 * indeed, if it tries it will most likely fail because dynamic memory
	 * allocation is not yet available.
	 **/
	// Arch specific CPU vector table setup.
	installVectorRoutines();
	// Arch specific exception handler setup.
	installExceptionRoutines();

	/* This is basic per-CPU level initialization. Per-CPU vector table
	 * installation. For the BSP CPU in FIRSTPLUG mode, it's done here.
	 * For the BSP CPU in HOTPLUG mode, it's done in the __kcpuPowerOn
	 * sequence.
	 **/
	bspCpu.initializeExceptions();
	return ERROR_SUCCESS;
}

error_t InterruptTrib::initializeIrqs(void)
{
	error_t		ret;

	/**	EXPLANATION:
	 * After a call to initializeIrqs(), the chipset is expected to
	 * be fully in synch with the kernel on all information to do with IRQ
	 * pins and bus/device <-> IRQ-pin relationships. The IBM-PC will be
	 * used as the paradigm for explanation:
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
	// Initialize the data structure before use.
	ret = pinIrqTable.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	zkcmCore.chipsetEventNotification(
		__KPOWER_EVENT_INTERRUPT_TRIB_AVAIL, 0);

	return ERROR_SUCCESS;
}

void InterruptTrib::pinIrqMain(RegisterContext *regs)
{
	sIrqPinDescriptor	*pinDescriptor;
	sIsrDescriptor		*isrDescriptor, (*isrRetireList[4]);
	status_t		status;
	ubit8			makeNoise, triggerMode, isrRetireListLength=0;
	ubit16			__kpin;

	// Ask the chipset if any pin-based IRQs are pending and handle them.
	status = zkcmCore.irqControl.identifyActiveIrq(
		cpuTrib.getCurrentCpuStream()->cpuId,
		regs->vectorNo,
		&__kpin, &triggerMode);

	makeNoise = (__kpin != 0 && __kpin != 18 && __kpin != 16);

	if (status == IRQCTL_IDENTIFY_ACTIVE_IRQ_SPURIOUS
		|| status ==  IRQCTL_IDENTIFY_ACTIVE_IRQ_UNIDENTIFIABLE)
	{
		if (status == IRQCTL_IDENTIFY_ACTIVE_IRQ_SPURIOUS)
		{
			printf(WARNING INTTRIB"pinIrqMain: Spurious IRQ: "
				"vector %d.\n",
				regs->vectorNo);
		}
		else
		{
			printf(WARNING INTTRIB"pinIrqMain: Unidentifiable "
				"interrupt on vector %d.\n"
				"\tMay not necessarily even be a pin-IRQ.\n",
				regs->vectorNo);
		};

		for (;FOREVER;)
		{
			cpuControl::disableInterrupts();
			cpuControl::halt();
		};
	};

	pinDescriptor = (sIrqPinDescriptor *)pinIrqTable.getItem(__kpin);
	if (pinDescriptor == NULL)
	{
		printf(WARNING INTTRIB"handlePinIrq: __kpin %d: No such "
			"__kpin descriptor.\n",
			__kpin);

		return;
	};

	if (makeNoise)
	{
		printf(NOTICE INTTRIB"Pin-based IRQ (__kpin %d) on CPU %d.\n"
			"\tDumping: %d unhandled, %d ISRs, %s triggered.\n",
			__kpin,
			cpuTrib.getCurrentCpuStream()->cpuId,
			pinDescriptor->nUnhandled,
			pinDescriptor->isrList.getNItems(),
			(triggerMode == IRQCTL_IRQPIN_TRIGGMODE_LEVEL)
				? "level" : "edge");
	};

	atomicAsm::set(&pinDescriptor->inService, 1);

	HeapList<sIsrDescriptor>::Iterator	it =
		pinDescriptor->isrList.begin(0);

	for (; it != pinDescriptor->isrList.end(); ++it)
	{
		isrDescriptor = *it;

		// For now only ZKCM.
		status = isrDescriptor->api.zkcm.isr(
			isrDescriptor->api.zkcm.device, 0);

		if (status != ZKCM_ISR_NOT_MY_IRQ)
		{
			if (status != ZKCM_ISR_SUCCESS
				&& status != ZKCM_ISR_SUCCESS_AND_RETIRE_ME)
			{
				// Else error.
				pinDescriptor->nUnhandled++;
				panic(FATAL"Error handling an IRQ.\n");
			};

			isrDescriptor->nHandled++;
			if (status == ZKCM_ISR_SUCCESS_AND_RETIRE_ME)
			{
				if (isrRetireListLength > 4)
				{
					printf(WARNING INTTRIB"__kpin %d: "
						"isrRetireList full.\n",
						__kpin);
				}
				else
				{
					isrRetireList[isrRetireListLength++] =
						isrDescriptor;
				};
			};

			if (triggerMode == IRQCTL_IRQPIN_TRIGGMODE_LEVEL) {
				break;
			};
		};
	};

	atomicAsm::set(&pinDescriptor->inService, 0);
	// FIXME: may want to avoid sending EOI for spurious IRQs.
	zkcmCore.irqControl.sendEoi(__kpin);

	// Run the retire list.
	if (isrRetireListLength == 0) { return; };
	for (uarch_t i=0; i<isrRetireListLength; i++)
	{
		if (isrRetireList[i]->driverType == sIsrDescriptor::ZKCM)
		{
			zkcm.retirePinIsr(
				__kpin, isrRetireList[i]->api.zkcm.isr);
		}
		else {};
	};
}

void InterruptTrib::exceptionMain(RegisterContext *regs)
{
	if (msiIrqTable[regs->vectorNo].type == sVectorDescriptor::UNCLAIMED)
	{
		printf(FATAL INTTRIB"Entry on UNCLAIMED vector %d "
			"from CPU %d.\n",
			regs->vectorNo,
			cpuTrib.getCurrentCpuStream()->cpuId);

		panic(ERROR_CRITICAL);
	};

	(*msiIrqTable[regs->vectorNo].exception)(regs, 0);

	// Check to see if the exception requires a "postcall".
	if (FLAG_TEST(
			msiIrqTable[regs->vectorNo].flags,
			INTTRIB_EXCEPTION_FLAGS_POSTCALL))
	{
		(msiIrqTable[regs->vectorNo].exception)(regs, 1);
	};
}

void InterruptTrib::installException(
	uarch_t vector, __kexceptionFn *exception, uarch_t flags)
{
	if (exception == NULL) { return; };
	if (msiIrqTable[vector].type != sVectorDescriptor::UNCLAIMED)
	{
		printf(FATAL INTTRIB"installException: Vector %d is already "
			"occupied.\n",
			vector);

		panic(ERROR_CRITICAL);
		return;
	};

	// Else it's a normal exception installation.
	msiIrqTable[vector].type = sVectorDescriptor::EXCEPTION;
	msiIrqTable[vector].exception = exception;
	msiIrqTable[vector].flags = flags;
}

error_t InterruptTrib::sZkcm::registerPinIsr(
	ubit16 __kpin, ZkcmDeviceBase *dev, zkcmIsrFn *isr, uarch_t /*flags*/
	)
{
	sIrqPinDescriptor	*pinDesc;
	sIsrDescriptor		*isrDesc;
	error_t			ret;

	/**	FIXME:
	 * Race condition here if one tries to register an ISR on a __kpin
	 * while the kernel is servicing an IRQ from that __kpin. The atomic
	 * inService flag used below is only a half-solution.
	 *
	 * Need an rwlock.
	 **/
	if (dev == NULL || isr == NULL) { return ERROR_INVALID_ARG; };

	pinDesc = (sIrqPinDescriptor *)interruptTrib.pinIrqTable
		.getItem(__kpin);

	if (pinDesc == NULL)
	{
		printf(ERROR INTTRIB"registerPinIsr: Invalid __kpin %d.\n",
			__kpin);

		return ERROR_INVALID_ARG_VAL;
	};

	while (atomicAsm::read(&pinDesc->inService) != 0) {
		cpuControl::subZero();
	};

	// Constructor zeroes it out.
	isrDesc = new sIsrDescriptor(InterruptTrib::sIsrDescriptor::ZKCM);
	if (isrDesc == NULL)
	{
		printf(ERROR INTTRIB"registerPinIsr: Failed to alloc "
			"sIsrDescriptor object.\n");

		return ERROR_MEMORY_NOMEM;
	};

	isrDesc->api.zkcm.isr = isr;
	isrDesc->api.zkcm.device = dev;

	ret = pinDesc->isrList.insert(isrDesc);
	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR INTTRIB"registerPinIsr: Failed to add new ISR "
			"on __kpin %d.\n\tZkcmDevice obj %p, ISR %p.\n",
			__kpin, dev, isr);

		delete isrDesc;
		return ERROR_GENERAL;
	};

	return ERROR_SUCCESS;
}

sarch_t InterruptTrib::sZkcm::retirePinIsr(ubit16 __kpin, zkcmIsrFn *isr)
{
	sIrqPinDescriptor	*pinDesc;
	sIsrDescriptor		*isrDesc;

	/**	FIXME:
	 * Race condition here if one tries to retire an ISR on a __kpin
	 * while the kernel is servicing an IRQ from that __kpin. The atomic
	 * inService flag used below is only a half-solution.
	 *
	 * Need an rwlock.
	 **/
	if (isr == NULL) { return 0; };

	pinDesc = (sIrqPinDescriptor *)interruptTrib.pinIrqTable
		.getItem(__kpin);

	if (pinDesc == NULL)
	{
		printf(ERROR INTTRIB"retirePinIsr: Invalid __kpin %d.\n",
			__kpin);

		return 0;
	};

	while (atomicAsm::read(&pinDesc->inService) != 0) {
		cpuControl::subZero();
	};

	HeapList<sIsrDescriptor>::Iterator	it = pinDesc->isrList.begin(0);

	for (; it != pinDesc->isrList.end(); ++it)
	{
		isrDesc = *it;

		if (isrDesc->api.zkcm.isr != isr) { continue; };

		// If we found the right one:
		printf(NOTICE INTTRIB"retirePinIsr: Retiring ISR %p on "
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

error_t InterruptTrib::__kpinEnable(ubit16 __kpin)
{
	sIrqPinDescriptor	*pinDesc;

	pinDesc = (sIrqPinDescriptor *)pinIrqTable.getItem(__kpin);
	if (pinDesc == NULL)
	{
		printf(ERROR INTTRIB"__kpinEnable: Invalid __kpin %d.\n",
			__kpin);

		return ERROR_INVALID_ARG_VAL;
	};

	if (pinDesc->isrList.getNItems() == 0)
	{
		printf(WARNING INTTRIB"__kpinEnable: %d: No ISRs in list.\n",
			__kpin);

		return ERROR_INVALID_OPERATION;
	};

	// Unmask the __kpin.
	zkcmCore.irqControl.unmaskIrq(__kpin);
	return ERROR_SUCCESS;
}

sarch_t InterruptTrib::__kpinDisable(ubit16 __kpin)
{
	sIrqPinDescriptor	*pinDesc;

	pinDesc = (sIrqPinDescriptor *)pinIrqTable.getItem(__kpin);
	if (pinDesc == NULL)
	{
		printf(ERROR INTTRIB"__kpinEnable: Invalid __kpin %d.\n",
			__kpin);

		return 0;
	};

	if (pinDesc->isrList.getNItems() != 0)
	{
		printf(WARNING INTTRIB"__kpinEnable: %d: Unable to mask "
			"__kpin: still %d handlers in list.\n",
			__kpin, pinDesc->isrList.getNItems());

		return 0;
	};

	// Mask the __kpin off.
	zkcmCore.irqControl.maskIrq(__kpin);
	return 1;
}

void InterruptTrib::removeException(uarch_t vector)
{
	if (msiIrqTable[vector].type != sVectorDescriptor::EXCEPTION)
	{
		printf(ERROR INTTRIB"removeException: Vector %d is not an "
			"exception vector.\n",
			vector);

		return;
	};

	msiIrqTable[vector].exception = NULL;
	msiIrqTable[vector].flags = 0;
	msiIrqTable[vector].type = sVectorDescriptor::UNCLAIMED;
}

void InterruptTrib::registerIrqPins(ubit16 nPins, sZkcmIrqPin *pinList)
{
	sIrqPinDescriptor	*tmp;
	error_t			err;

	for (ubit16 i=0; i<nPins; i++)
	{
		tmp = new sIrqPinDescriptor;
		tmp->isrList.initialize();
		if (tmp == NULL)
		{
			printf(FATAL INTTRIB"registerIrqPins(): Failed to "
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
		 * HardwareIdList::findFreeIndex() to re-use indexes in the
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
			printf(FATAL INTTRIB"registerPinIrqs(): "
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

void InterruptTrib::removeIrqPins(ubit16 nPins, sZkcmIrqPin *pinList)
{
	sIrqPinDescriptor		*tmp;

	for (ubit16 i=0; i<nPins; i++)
	{
		tmp = reinterpret_cast<sIrqPinDescriptor *>(
			pinIrqTable.getItem(pinList[i].__kid) );

		// TODO: Remove each handler registered on the pin as well.
		pinIrqTable.removeItem(pinList[i].__kid);
		delete tmp;
	};
}

void InterruptTrib::dumpExceptions(void)
{
	ubit8		flipFlop=0;

	printf(NOTICE INTTRIB"dumpExceptions:\n");
	for (ubit16 i=0; i<ARCH_INTERRUPTS_NVECTORS; i++)
	{
		if (msiIrqTable[i].type == sVectorDescriptor::EXCEPTION)
		{
			printf(CC"\t%d: %p,", i, msiIrqTable[i].exception);
			if (flipFlop == 3)
			{
				printf(CC"\n");
				flipFlop = 0;
			}
			else { flipFlop++; };
		};
	};
	if (flipFlop != 0) { printf(CC"\n"); };
}

void dumpIsrDescriptor(ubit16 num, InterruptTrib::sIsrDescriptor *desc)
{
	if (desc->driverType == InterruptTrib::sIsrDescriptor::ZKCM)
	{
		printf(CC"\t%d (is ZKCM): nHandled %d,"
			"\n\tisr @%p, device base @%p.\n",
			num, desc->nHandled,
			desc->api.zkcm.isr, desc->api.zkcm.device);
	};
}

void InterruptTrib::dumpMsiIrqs(void)
{
	ubit32		nItems;
	sIsrDescriptor	*tmp;

	printf(NOTICE INTTRIB"dumpMsiIrqs:\n");

	for (ubit16 i=0; i<ARCH_INTERRUPTS_NVECTORS; i++)
	{
		if (msiIrqTable[i].type == sVectorDescriptor::MSI_IRQ)
		{
			nItems = msiIrqTable[i].isrList.getNItems();
			printf(CC"vec%d: %d handlers.\n", i, nItems);
			for (ubit16 j=0; j<nItems; j++)
			{
				tmp = msiIrqTable[i].isrList.getItem(j);
				dumpIsrDescriptor(j, tmp);
			};
		};
	};
}

void InterruptTrib::dumpUnusedVectors(void)
{
	ubit8	flipFlop=0;

	printf(NOTICE INTTRIB"dumpUnusedVectors:\n");
	for (ubit16 i=0; i<ARCH_INTERRUPTS_NVECTORS; i++)
	{
		if (msiIrqTable[i].type == sVectorDescriptor::UNCLAIMED)
		{
			printf(CC"\t%d,", i);
			if (flipFlop == 7)
			{
				flipFlop = 0;
				printf(CC"\n");
			}
			else { flipFlop++; };
		};
	};
	if (flipFlop != 0) { printf(CC"\n"); };
}

void InterruptTrib::dumpIrqPins(void)
{
	HardwareIdList::Iterator	curr;
	sIrqPinDescriptor		*tmp;

	printf(NOTICE INTTRIB"dumpIrqPins:\n");

	curr = pinIrqTable.begin();
	for (; curr != pinIrqTable.end(); ++curr)
	{
		tmp = reinterpret_cast<sIrqPinDescriptor *>( *curr );

		printf(CC"\t__kpin %d: %d devices.\n",
			curr.cursor, tmp->isrList.getNItems());
	};
}

