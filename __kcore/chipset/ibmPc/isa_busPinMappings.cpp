
#include <chipset/zkcm/zkcmCore.h>
#include <__kstdlib/__ktypes.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kclasses/debugPipe.h>
#include <commonlibs/libacpi/libacpi.h>
#include <commonlibs/libx86mp/libx86mp.h>
#include <kernel/common/panic.h>

#include "zkcmIbmPcState.h"
#include "busPinMappings.h"
#include "i8259a.h"


/**	EXPLANATION:
 * This file contains most of the code that handles translating bus-specific
 * IRQ numbers into __kpin IDs, and programming interrupt controllers for the
 * buses in question. In less Zambesii specific terms, this file does
 * most of the work related to determining which IO-APIC or i8259 controller
 * pins each IRQ signal from each bus is connected to.
 *
 * Since this is specifically the bus-pin mapping code for the IBM-PC, there are
 * to date only three buses which will need to have their IRQs decoded in here:
 * ISA, PCI and ACPI. ACPI is technically not a bus, but it has an IRQ (SCI)
 * which is distinct in source from other buses, and is most smoothly handled
 * as a separate bus which requires IRQ signal-to-pic-pin mapping.
 *
 *	ISA IRQs:
 * If the chipset is not in SMP mode (and is therefore using the i8259s and not
 * the IO-APICs) we assume that all ISA IRQs directly apply to the i8259 pin
 * of that number.
 *
 * If the chipset is in SMP mode (and has therefore switched over to using the
 * IO-APICS) we detect where ISA IRQs are mapped on the IOAPIC's pins using the
 * Intel MP Specification tables, and the ACPI MADT table's IRQ Source Override
 * entries (preferred).
 *
 *	The ACPI SCI IRQ signal:
 * If the chipset is not in SMP mode, then we can take the value for the SCI
 * IRQ pin directly from the FADT (SCI_INT value) as being a pin on the i8259.
 *
 * If the chipset is in SMP mode (and is therefore using the IO-APICs) we first
 * detect which IO-APIC pin each ISA IRQ is attached to, then we read the
 * SCI_INT value from the FADT, and translate that value into the correct
 * IO-APIC pin using the ISA mapping information.
 *
 *	PCI IRQs:
 * Detecting where PCI Links are attached to the interrupt controllers on a
 * chipset is relatively complex, and will probably get more complex as time
 * goes on.
 *
 * 1. Chipset is being used in non-SMP mode (i8259 controller mode).
 *	* Method 1: (Ideal): Execute the ACPI _PIC() method, with "PIC" mode
 *	  as the argument. Then populate the ACPI namespace and detect the
 *	  PCI _PRT() objects in the tree.
 *	* Method 2: Use the $PRT table in RAM to determine where each PCI link
 *	  is connected on the i8259s.
 *	* Method 3: Use the PCI BIOS functions to return information on where
 *	  each PCI link is connected on the i8259s.
 *
 * 2. Chipset is being used in SMP mode (IO-APIC mode).
 *	* Method 1: (Ideal): Execute the ACPI _PIC() method, telling the
 *	  firmware that we want to use symmetric IO mode. Populate the ACPI
 *	  namespace and find the _PRT() objects in the tree.
 *	* Method 2: (Acceptable): Look in RAM for an Intel MP Specification table
 *	  and parse it looking for IRQ Source entries pertaining to PCI. Use
 *	  this information to infer where each PCI link is connected on the
 *	  IO-APICs.
 **/

struct sIsaBusPinMapping
{
	ubit8		isValid;
	ubit16		__kpin;
} isaBusPinMappings[16];

static ubit8 loadedSmpIsaBusPinMappingsAlready=0;

static void ibmPc_isaBpm_smpMode_acpi_setIsaDefaults(void)
{
	x86IoApic::IoApic	*ioApic;
	ubit8			cpu, dummy;

	/**	EXPLANATION:
	 * In synopsis, set the IO-APIC pins that are used by the ISA IRQs to
	 * ISA defaults (edge-triggered, rising), and generally explicitly
	 * program each pin to interrupt on the correct vector. If this is the
	 * first time we're running through this sequence, we set each ISA pin
	 * to interrupt the BSP CPU. Else, we preserve the CPU that the pin
	 * is currently set to interrupt on, as set by the kernel.
	 *
	 * This sequence is necessary because the ACPI approach to handling ISA
	 * IRQs is to assume that all ISA IRQs are ID-mapped to GIRQs 0-15.
	 * There may be no overrides in the MADT at all, so if we don't run
	 * through and explicitly initialize every pin, the majority of them
	 * will be uninitialized.
	 *
	 * Those pins which must have overrides or non-default settings applied
	 * (level triggered, etc) will be processed as per directives in the
	 * MADT override entries.
	 **/

	/* Fill out each ISA IRQ as being ID mapped to the ACPI global IRQ with
	 * the same number.
	 **/
	for (ubit8 girq=0; girq<16; girq++)
	{
		if (x86IoApic::get__kpinFor(
			girq, &isaBusPinMappings[girq].__kpin)
			== ERROR_SUCCESS)
		{
			ubit8		pin;

			isaBusPinMappings[girq].isValid = 1;

			ioApic = x86IoApic::getIoApicFor(
				isaBusPinMappings[girq].__kpin);

			assert_fatal(ioApic != NULL);

			ioApic->lookupPinBy__kid(
				isaBusPinMappings[girq].__kpin, &pin);

			assert_fatal(pin < ioApic->getNIrqs());

			/* Here we set each pin to a default state when loading
			 * bus-pin information and pin-programming info.
			 *
			 * If the ISA bus-pin mappings are being loaded for the
			 * first time, we set the CPU for each pin to the BSP's
			 * ID. Else, we have to preserve the ID of the CPU
			 * that the pin currently maps to since that was most
			 * likely set by the kernel as part of IRQ load
			 * balancing or power management.
			 **/
			if (loadedSmpIsaBusPinMappingsAlready)
			{
				ioApic->getPinState(
					pin, &cpu, &dummy,
					&dummy, &dummy, &dummy, &dummy);
			}
			else {
				cpu = ibmPcState.bspInfo.bspId;
			};

			ioApic->setPinState(
				pin, cpu, ioApic->getVectorBase() + pin,
				x86IOAPIC_DELIVERYMODE_FIXED,
				x86IOAPIC_DESTMODE_PHYSICAL,
				x86IOAPIC_POLARITY_HIGH,
				x86IOAPIC_TRIGGMODE_EDGE);
		}
		else
		{
			printf(FATAL IBMPCBPM"Unable to get "
				"__kpinId for IO-APIC GIRQ %d.\n",
				girq);

			isaBusPinMappings[girq].isValid = 0;
		};
	};
}

void dumpSmpIsaPins(void)
{
	printf(NOTICE IBMPCBPM"Dumping all ISA SMP IRQs.\n");
	for (uarch_t girq=0; girq<16; girq++)
	{
		ubit16			__kpin;
		x86IoApic::IoApic	*ioApic;
		ubit8			enabled;
		ubit8			pin, cpu, vector, polarity, triggMode,
					destMode, deliveryMode;

		// ISA-GIRQ -> __kpin.
		x86IoApic::get__kpinFor(girq, &__kpin);
		ioApic = x86IoApic::getIoApicFor(__kpin);
		assert_fatal(ioApic != NULL);

		ioApic->lookupPinBy__kid(__kpin, &pin);
		enabled = ioApic->getPinState(
			pin, &cpu, &vector,
			&deliveryMode, &destMode,
			&polarity, &triggMode);

		printf(NOTICE"\tGIRQ-%d (IoApic %d,%d): %s; cpu%d, v %d.\n",
			girq, ioApic->getId(), pin,
			(enabled) ? "on" : "off",
			cpu, vector);
	};
}

static error_t ibmPc_isaBpm_smpMode_rsdt_loadBusPinMappings(void)
{
	acpi::sRsdt			*rsdt;
	acpiR::sMadt			*madt;
	acpiR::madt::sIrqSourceOver	*irqOverride;
	void				*context, *handle, *handle2;
	ubit32				nOverrides=0;

	ibmPc_isaBpm_smpMode_acpi_setIsaDefaults();

	rsdt = acpi::getRsdt();

	handle = context = NULL;
	// We'll only use the first MADT we encounter.
	madt = acpiRsdt::getNextMadt(rsdt, &context, &handle);
	if (madt == NULL)
	{
		printf(WARNING IBMPCBPM"ISA: No MADTs found.\n");
		return ERROR_UNSUPPORTED;
	};

	/* The aim here is to find at least one "Interrupt Source
	 * Override" structure in the MADT. If we find one, we will
	 * use the information in the MADT in preference to the MP
	 * Tables.
	 *
	 * If we find no interrupt source override entries in the MADT,
	 * we will check the MP Tables to see if it has any. If it has
	 * at least one, we will use the MP Tables instead. If however
	 * the MP Tables hold no override entries as well, we will
	 * assume that on this chipset, all ISA IRQs are identity
	 * mapped to the IO-APIC IRQ pins (0-15).
	 **/
	handle2 = NULL;
	irqOverride = acpiR::madt::getNextIrqSourceOverrideEntry(madt, &handle2);
	for (;
		irqOverride != NULL;
		irqOverride = acpiR::madt::getNextIrqSourceOverrideEntry(
			madt, &handle2))
	{
		ubit8			pin, cpu, dummy, polarity, triggerMode;
		x86IoApic::IoApic	*ioApic;

		nOverrides++;

		if (irqOverride->irqNo > 15)
		{
			printf(ERROR IBMPCBPM"Illegal ISA IRQ number %d "
				"found in MADT ISA IRQ Source overrides. "
				"Skipping.\n",
				irqOverride->irqNo);

			continue;
		};

		if (x86IoApic::get__kpinFor(
			irqOverride->globalIrq,
			&isaBusPinMappings[irqOverride->irqNo].__kpin)
				!= ERROR_SUCCESS)
		{
			printf(FATAL IBMPCBPM"Unable to get __kpin ID for "
				"IO-APIC pin %d.\n",
				irqOverride->globalIrq);

			continue;
		};

		ioApic = x86IoApic::getIoApicFor(
			isaBusPinMappings[irqOverride->irqNo].__kpin);

		if (ioApic == NULL)
		{
			printf(FATAL IBMPCBPM"ISA: LibIO-APIC claims to "
				"have a __kpin mapping\n"
				"\tfor ACPI GIRQ %d, but at the same time, no "
				"IO-APIC is returned when looking\n"
				"\tup __kpin %d.\n",
				irqOverride->globalIrq,
				isaBusPinMappings[irqOverride->irqNo].__kpin);

			panic(ERROR_FATAL);
		};

		// Program the IO-APIC pin now.
		assert_fatal(
			ioApic->lookupPinBy__kid(
				isaBusPinMappings[irqOverride->irqNo].__kpin,
				&pin)
				== ERROR_SUCCESS);

		switch (irqOverride->flags & ACPI_MADT_IRQSRCOVER_POLARITY_MASK)
		{
		case ACPI_MADT_IRQSRCOVER_POLARITY_ACTIVELOW:
			printf(WARNING IBMPCBPM"ISA: Unusual polarity low "
				"for IRQ %d.\n",
				irqOverride->irqNo);

			polarity = x86IOAPIC_POLARITY_LOW;
			break;

		default: polarity = x86IOAPIC_POLARITY_HIGH; break;
		};

		switch (irqOverride->flags
			>> ACPI_MADT_IRQSRCOVER_TRIGGER_SHIFT
			& ACPI_MADT_IRQSRCOVER_TRIGGER_MASK)
		{
		case ACPI_MADT_IRQSRCOVER_TRIGGER_LEVEL:
			printf(WARNING IBMPCBPM"ISA: Unusual trigger level "
				"for IRQ %d.\n",
				irqOverride->irqNo);

			triggerMode = x86IOAPIC_TRIGGMODE_LEVEL; break;
		default: triggerMode = x86IOAPIC_TRIGGMODE_EDGE; break;
		};

		// Only care about preserving the pin's CPU here.
		ioApic->getPinState(
			pin, &cpu, &dummy,
			&dummy, &dummy, &dummy, &dummy);

		// Have to call this to set the polarity/triggmode.
		ioApic->setPinState(
			pin, cpu, ioApic->getVectorBase() + pin,
			x86IOAPIC_DELIVERYMODE_FIXED,
			x86IOAPIC_DESTMODE_PHYSICAL,
			polarity, triggerMode);

		ioApic->setIrqStatus(
			isaBusPinMappings[irqOverride->irqNo].__kpin,
			cpu, ioApic->getVectorBase() + pin, 0);

		ioApic->irqPinList[pin].triggerMode =
			(triggerMode == x86IOAPIC_TRIGGMODE_LEVEL)
				? IRQCTL_IRQPIN_TRIGGMODE_LEVEL
				: IRQCTL_IRQPIN_TRIGGMODE_EDGE;

		ioApic->irqPinList[pin].polarity =
			(polarity == x86IOAPIC_POLARITY_LOW)
				? IRQCTL_IRQPIN_POLARITY_LOW
				: IRQCTL_IRQPIN_POLARITY_HIGH;

		printf(NOTICE IBMPCBPM"ACPI override: mapped ISA IRQ "
			"%d to __kpin %d (girq: %d).\n",
			irqOverride->irqNo,
			isaBusPinMappings[irqOverride->irqNo].__kpin,
			irqOverride->globalIrq);
	};

	acpiRsdt::destroySdt((acpi::sSdt *)madt);
	acpiRsdt::destroyContext(&context);

	return ERROR_SUCCESS;
}

static error_t ibmPc_isaBpm_smpMode_x86Mp_loadBusPinMappings(void)
{
	x86Mp::sIrqSourceConfig	*irqSourceEntry;
	uarch_t			pos;
	void			*handle;
	x86IoApic::IoApic	*ioApic;
	sbit8			isaBusId;
	// sbit8		eisaBusId;

	isaBusId = x86Mp::getBusIdFor(x86_MPCFG_BUS_ISA);
	if (isaBusId < 0)
	{
		printf(WARNING IBMPCBPM"x86MP: Unable to determine the ID "
			"given to the ISA bus by firmware.\n");

		return ERROR_UNKNOWN;
	};
	// eisaBusId = x86Mp::getBusIdFor(x86_MPCFG_BUS_EISA);
	printf(NOTICE IBMPCBPM"x86MP ISA BUS ID: %d.\n", isaBusId);

	pos = 0;
	handle = NULL;
	irqSourceEntry = x86Mp::getNextIrqSourceEntry(&pos, &handle);

	for (; irqSourceEntry != NULL;
		irqSourceEntry = x86Mp::getNextIrqSourceEntry(&pos, &handle))
	{
		ubit8		cpu, dummy, polarity, triggerMode;

		if (irqSourceEntry->sourceBusId != isaBusId /*
			&& irqSourceEntry->sourceBusId != eisaBusId*/)
		{
			// Only interested in IRQs from the ISA bus currently.
			continue;
		};

		if (irqSourceEntry->sourceBusIrq > 15)
		{
			printf(FATAL IBMPCBPM"x86MP: Invalid ISA IRQ number "
				"%d.\n",
				irqSourceEntry->sourceBusIrq);

			continue;
		};

		/**	NOTES:
		 * For each IRQ from the ISA bus:
		 *	* Use LibIoApic to retrieve the __kpin ID of the IRQ
		 *	  based on the "IO-APIC-ID + pin-offset" information
		 *	  given per IRQ in the table.
		 *	* Fill out the correct __kpin ID for the ISA IRQ in the
		 *	  isaBusPinMapping[] array.
		 **/
		ioApic = x86IoApic::getIoApic(irqSourceEntry->destIoApicId);
		if (ioApic == NULL)
		{
			printf(FATAL IBMPCBPM"x86MP: Serious: MP Tables "
				"speak of an IO-APIC %d, but\n\tthe kernel "
				"does not have an IO-APIC object\n\tto manage "
				"it.\n",
				irqSourceEntry->destIoApicId);

			panic(ERROR_UNKNOWN);
		};

		isaBusPinMappings[irqSourceEntry->sourceBusIrq].__kpin =
			ioApic->get__kpinBase() + irqSourceEntry->destIoApicPin;

		isaBusPinMappings[irqSourceEntry->sourceBusIrq].isValid = 1;

		if (loadedSmpIsaBusPinMappingsAlready)
		{
			// We don't care for anything but the pin's CPU here.
			ioApic->getPinState(
				irqSourceEntry->destIoApicPin, &cpu,
				&dummy, &dummy, &dummy, &dummy, &dummy);
		}
		else {
			cpu = ibmPcState.bspInfo.bspId;
		};

		// Determine and set the correct polarity and trigger-mode, etc.
		switch (irqSourceEntry->flags
			& x86_MPCFG_IRQSRC_FLAGS_POLARITY_MASK)
		{
		case x86_MPCFG_IRQSRC_POLARITY_ACTIVELOW:
			printf(WARNING IBMPCBPM"ISA: Unusual polarity low "
				"for IRQ %d.\n",
				irqSourceEntry->sourceBusIrq);

			polarity = x86IOAPIC_POLARITY_LOW;
			break;

		default: polarity = x86IOAPIC_POLARITY_HIGH; break;
		};

		switch (irqSourceEntry->flags
			>> x86_MPCFG_IRQSRC_FLAGS_SENSITIVITY_SHIFT
			& x86_MPCFG_IRQSRC_FLAGS_SENSITIVITY_MASK)
		{
		case x86_MPCFG_IRQSRC_SENSITIVITY_LEVEL:
			printf(WARNING IBMPCBPM"ISA: Unusual trigger level "
				"for IRQ %d.\n",
				irqSourceEntry->sourceBusIrq);

			triggerMode = x86IOAPIC_TRIGGMODE_LEVEL;
			break;

		default: triggerMode = x86IOAPIC_TRIGGMODE_EDGE; break;
		};

		/* Can only set polarity, triggmode, dest and delivery mode
		 * using setPinState(), and not using setIrqStatus();
		 * setIrqStatus() does not take arguments for those parameters.
		 **/
		ioApic->setPinState(
			irqSourceEntry->destIoApicPin, cpu,
			ioApic->getVectorBase() + irqSourceEntry->destIoApicPin,
			x86IOAPIC_DELIVERYMODE_FIXED,
			x86IOAPIC_DESTMODE_PHYSICAL,
			polarity, triggerMode);

		ioApic->setIrqStatus(
			isaBusPinMappings[irqSourceEntry->sourceBusIrq].__kpin,
			cpu,
			ioApic->getVectorBase() + irqSourceEntry->destIoApicPin,
			0);

		ioApic->irqPinList[irqSourceEntry->sourceBusIrq].triggerMode =
			(triggerMode == x86IOAPIC_TRIGGMODE_LEVEL)
				? IRQCTL_IRQPIN_TRIGGMODE_LEVEL
				: IRQCTL_IRQPIN_TRIGGMODE_EDGE;

		ioApic->irqPinList[irqSourceEntry->sourceBusIrq].polarity =
			(polarity == x86IOAPIC_POLARITY_LOW)
				? IRQCTL_IRQPIN_POLARITY_LOW
				: IRQCTL_IRQPIN_POLARITY_HIGH;

		printf(NOTICE IBMPCBPM"x86MP: Mapped ISA IRQ %d to __kpin "
			"%d (IO-APIC %d, pin %d).\n",
			irqSourceEntry->sourceBusIrq,
			isaBusPinMappings[irqSourceEntry->sourceBusIrq].__kpin,
			irqSourceEntry->destIoApicId,
			irqSourceEntry->destIoApicPin);
	};

	return ERROR_SUCCESS;
}

status_t ibmPc_isaBpm_smpMode_loadBusPinMappings(void)
{
	status_t		ret;

	/**	EXPLANATION:
	 * The chipset is in SMP mode; we can get the correct bus-pin
	 * mapping information from either:
	 *	1. ACPI (preferred).
	 *	2. MP Tables.
	 *
	 * Try ACPI first, then try MP tables if that fails.
	 **/
	acpi::initializeCache();
	ret = acpi::findRsdp();
	if (ret != ERROR_SUCCESS)
	{
		printf(NOTICE IBMPCBPM"ISA: Failed to find RSDP.\n" );
		goto tryMpTables;
	};

#if !defined(__32_BIT__) || defined(CONFIG_ARCH_x86_32_PAE)
	if (rsdp::testForXsdt())
	{
		ret = rsdp::mapXsdt();
		if (ret != ERROR_SUCCESS)
		{
			printf(NOTICE IBMPCBPM"ISA: Failed to map XSDT.\n"
				"\tFalling back to RSDT.\n");

			goto useRsdt;
		};

		/* printf(NOTICE IBMPCBPM"Using ACPI XSDT for SMP-mode ISA "
			"bus-pin mappings.\n");

		ret = ibmPc_isaBpm_smpMode_xsdt_loadBusPinMappings();
		if (ret != ERROR_SUCCESS)
		{
			printf(WARNING IBMPCBPM"ISA: XSDT present, but no "
				"Bus-pin mapping info.\n");

			// Fall through to testForRsdt().
		}
		else
		{
			loadedSmpIsaBusPinMappingsAlready = 1;
			return ret;
		}; */
	};

useRsdt:
#endif
	if (acpi::testForRsdt())
	{
		ret = acpi::mapRsdt();
		if (ret != ERROR_SUCCESS)
		{
			printf(NOTICE IBMPCBPM"ISA: Failed to map RSDT.\n");
			goto tryMpTables;
		};

		printf(NOTICE IBMPCBPM"Using RSDT for SMP-mode ISA Bus-pin "
			"mappings.\n");

		ret = ibmPc_isaBpm_smpMode_rsdt_loadBusPinMappings();
		if (ret != ERROR_SUCCESS)
		{
			printf(WARNING IBMPCBPM"ISA: RSDT present, but no "
				"Bus-pin mapping info.\n");

			// Fall through to MP Table tests.
		}
		else
		{
			loadedSmpIsaBusPinMappingsAlready = 1;
			return ret;
		};
	};

tryMpTables:
	x86Mp::initializeCache();
	if (x86Mp::findMpFp() == NULL)
	{
		printf(FATAL IBMPCBPM"Anomaly:\n"
			"\tYour chipset claims to support IO-APICs and SMP\n"
			"\tmode but did not supply ACPI IRQ Source Overrides,\n"
			"\tor even an MP Specification table.\n"
			"\tThe kernel cannot reliably determine where the ISA\n"
			"\tIRQs are mapped on the IO-APICs. Panicking.\n");

		panic(ERROR_UNKNOWN);
	};

	if (x86Mp::mapMpConfigTable() == NULL)
	{
		panic(ERROR IBMPCBPM"Failed to map MP Config table. There "
			"is a possibility that\n\tthere was bus->pin mapping "
			"info in there that was\n\tmissed. Halting.\n");
	};

	printf(NOTICE IBMPCBPM"Using x86 MP tables for SMP-mode ISA bus-pin "
		"mappings.\n");

	if (ibmPc_isaBpm_smpMode_x86Mp_loadBusPinMappings() != ERROR_SUCCESS)
	{
		panic(FATAL IBMPCBPM"Unable to read bus pin mappings from MP "
			"Tables.\n");
	};

	loadedSmpIsaBusPinMappingsAlready = 1;
	return ERROR_SUCCESS;
}

status_t ibmPcBpm::isa::loadBusPinMappings(void)
{
	/**	EXPLANATION:
	 * This function fills out the array above, isaBusPinMappings[] with
	 * the most current information available on the mapping of ISA devices
	 * to the IRQ subsystem currently in use.
	 *
	 * If the i8259s are in use, this function will assume that all ISA IRQs
	 * are mapped 1:1 to the i8259 inputs.
	 *
	 * If the IO-APIC IRQ routing subsystem is in use, this function will
	 * consult with the ACPI MADT (preferred) and the MP Tables to determine
	 * which IO-APIC input pins the ISA IRQs have been wired to.
	 **/
	if (ibmPcState.smpInfo.chipsetState == SMPSTATE_UNIPROCESSOR)
	{
		// Assume that all ISA IRQs are mapped 1:1 to the i8259 pins.
		for (ubit8 isaIrqNo=0; isaIrqNo<16; isaIrqNo++)
		{
			if (i8259aPic.get__kpinFor(
				isaIrqNo, &isaBusPinMappings[isaIrqNo].__kpin)
				!= ERROR_SUCCESS)
			{
				printf(FATAL IBMPCBPM"Unable to get "
					"__kpinId for i8259 pin %d.\n",
					isaIrqNo);

				isaBusPinMappings[isaIrqNo].isValid = 0;
			}
			else {
				isaBusPinMappings[isaIrqNo].isValid = 1;
			};
		};

		printf(NOTICE IBMPCBPM"ISA: uniprocessor mode. Assumed "
			"i8259s.\n");
		return ERROR_SUCCESS;
	};

	return ibmPc_isaBpm_smpMode_loadBusPinMappings();
}

error_t ibmPcBpm::isa::get__kpinFor(ubit32 busIrqId, ubit16 *__kpin)
{
	if (busIrqId > 15) { return ERROR_INVALID_ARG_VAL; };

	// Just lookup the __kpin in the table.
	if (isaBusPinMappings[busIrqId].isValid)
	{
		*__kpin = isaBusPinMappings[busIrqId].__kpin;
		return ERROR_SUCCESS;
	};

	return ERROR_RESOURCE_UNAVAILABLE;
}

#if 0
/* Deprecated in favour of forcing all calls to maskIrq() and unmaskIrq() to
 * go through Interrupt Trib and the IRQ Control module.
 **/
status_t ibmPcBpm::isa::maskIrq(ubit32 busIrqId)
{
	ubit16		__kpin;
	error_t		ret;

	ret = ibmPcBpm::isa::get__kpinFor(busIrqId, &__kpin);
	if (ret != ERROR_SUCCESS) {
		return ret;
	};

	zkcmCore.irqControl.maskIrq(__kpin);
	return ERROR_SUCCESS;
}

status_t ibmPcBpm::isa::unmaskIrq(ubit32 busIrqId)
{
	ubit16		__kpin;
	error_t		ret;

	ret = ibmPcBpm::isa::get__kpinFor(busIrqId, &__kpin);
	if (ret != ERROR_SUCCESS) {
		return ret;
	};

	zkcmCore.irqControl.unmaskIrq(__kpin);
	return ERROR_SUCCESS;
}
#endif

