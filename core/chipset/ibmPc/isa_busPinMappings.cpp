
#include <__kstdlib/__ktypes.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kclasses/debugPipe.h>
#include <commonlibs/libacpi/libacpi.h>
#include <commonlibs/libx86mp/libx86mp.h>
#include <kernel/common/panic.h>

#include "zkcmIbmPcState.h"
#include "irqControl.h"
#include "busPinMappings.h"


/**	EXPLANATION:
 * This file contains most of the code that handles translating bus-specific
 * IRQ numbers into __kpin IDs. In less Zambesii specific terms, this file does
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
 *	* Method 2: (Preferred): Look in RAM for an Intel MP Specification table
 *	  and parse it looking for IRQ Source entries pertaining to PCI. Use
 *	  this information to infer where each PCI link is connected on the
 *	  IO-APICs.
 **/

struct isaBusPinMappingS
{
	ubit8		isValid;
	ubit16		__kpin;
} isaBusPinMappings[16];

static sarch_t ibmPc_bpm_isa_acpi_checkRsdtForIsaMadtInfo(acpi_rMadtS **ret)
{
	acpi_rsdtS	*rsdt;
	acpi_rMadtS	*madt;
	void		*handle, *context, *handle2;

	*ret = __KNULL;
	rsdt = acpi::getRsdt();

	handle = context = __KNULL;
	// We'll only use the first MADT we encounter.
	madt = acpiRsdt::getNextMadt(rsdt, &context, &handle);
	if (madt != __KNULL)
	{
		handle2 = __KNULL;
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
		if (acpiRMadt::getNextIrqSourceOverrideEntry(madt, &handle2)
			!= __KNULL)
		{
			/* Only destroy the walk context before returning, and
			 * not the MADT. The MADT will be used by again by the
			 * caller.
			 **/
			acpiRsdt::destroyContext(&context);
			// Return the MADT that has been mapped into vmem.
			*ret = madt;
			return 1;
		};

		acpiRsdt::destroySdt(reinterpret_cast<acpi_sdtS *>( madt ));
		acpiRsdt::destroyContext(&context);
		return 0;
	};

	return 0;
}

static void ibmPc_bpm_isa_acpi_loadRsdtBusPinMappings(acpi_rMadtS *madt)
{
	acpi_rMadtIrqSourceOverS	*overrideEntry;
	void		*handle;

	/**	EXPLANATION:
	 * When using the ACPI IRQ Source Override entries, we need to first
	 * set every mapping to "valid", because we must, according to the ACPI
	 * specification, assume first that all ISA IRQs are identity mapped to
	 * the ACPI global IRQ namespace.
	 *
	 * Then, we overwrite the default configuration with the overrides in
	 * the MADT Override entries.
	 **/

	/* Fill out each ISA IRQ as being ID mapped to the ACPI global IRQ with
	 * the same number.
	 **/
	for (ubit8 i=0; i<16; i++)
	{
		if (ibmPc_irqControl_identifyIrq(
			i, &isaBusPinMappings[i].__kpin)
			!= ERROR_SUCCESS)
		{
			__kprintf(FATAL IBMPCBPM"Unable to get "
				"__kpinId for IO-APIC pin %d.\n",
				i);

			isaBusPinMappings[i].isValid = 0;
		}
		else {
			isaBusPinMappings[i].isValid = 1;
		};
	};


	handle = __KNULL;
	overrideEntry = acpiRMadt::getNextIrqSourceOverrideEntry(
		madt, &handle);

	/* Next, use the extra information in the MADT override entries to
	 * get the true state of those ISA IRQs which are in fact *not* ID
	 * mapped. A very common case is where most chipsets remap ISA IRQ 0 to
	 * ACPI global IRQ 2.
	 **/
	for (; overrideEntry != __KNULL;
		overrideEntry = acpiRMadt::getNextIrqSourceOverrideEntry(
				madt, &handle))
	{
		if (overrideEntry->irqNo > 15)
		{
			__kprintf(ERROR IBMPCBPM"Illegal ISA IRQ number %d "
				"found in MADT ISA IRQ Source overrides. "
				"Skipping.\n",
				overrideEntry->irqNo);

			continue;
		};

		if (ibmPc_irqControl_identifyIrq(
			overrideEntry->globalIrq,
			&isaBusPinMappings[overrideEntry->irqNo].__kpin)
				!= ERROR_SUCCESS)
		{
			__kprintf(FATAL IBMPCBPM"Unable to get __kpin ID for "
				"IO-APIC pin %d.\n",
				overrideEntry->globalIrq);

			continue;
		};

		isaBusPinMappings[overrideEntry->irqNo].isValid = 1;
		__kprintf(NOTICE IBMPCBPM"ACPI override: mapping ISA IRQ "
			"%d to __kpin %d (girq: %d).\n",
			overrideEntry->irqNo,
			isaBusPinMappings[overrideEntry->irqNo].__kpin,
			overrideEntry->globalIrq);
	};
}

static error_t ibmPc_bpm_isa_x86Mp_loadBusPinMappings(void)
{
	x86_mpCfgS		*cfg;
	x86_mpCfgIrqSourceS	*irqSourceEntry;
	uarch_t			pos;
	void			*handle;
	x86IoApic::ioApicC	*ioApic;
	sbit8			isaBusId;
	// sbit8		eisaBusId;
	error_t			ret;

	ret = ERROR_SUCCESS;
	cfg = x86Mp::getMpCfg();
	isaBusId = x86Mp::getBusIdFor(x86_MPCFG_BUS_ISA);
	if (isaBusId < 0)
	{
		__kprintf(WARNING IBMPCBPM"x86MP: Unable to determine the ID "
			"given to the ISA bus by firmware.\n");

		return ERROR_UNKNOWN;
	};
	// eisaBusId = x86Mp::getBusIdFor(x86_MPCFG_BUS_EISA);
	__kprintf(NOTICE IBMPCBPM"x86MP ISA BUS ID: %d.\n", isaBusId);

	pos = 0;
	handle = __KNULL;
	irqSourceEntry = x86Mp::getNextIrqSourceEntry(&pos, &handle);

	for (; irqSourceEntry != __KNULL;
		irqSourceEntry = x86Mp::getNextIrqSourceEntry(&pos, &handle))
	{
		if (irqSourceEntry->sourceBusId != isaBusId /*
			&& irqSourceEntry->sourceBusId != eisaBusId*/)
		{
			// Only interested in IRQs from the ISA bus currently.
			continue;
		};

		if (irqSourceEntry->sourceBusIrq > 15)
		{
			__kprintf(FATAL IBMPCBPM"x86MP: Invalid ISA IRQ number "
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
		if (ioApic == __KNULL)
		{
			__kprintf(FATAL IBMPCBPM"x86MP: Serious: MP Tables "
				"speak of an IO-APIC %d, but\n\tthe kernel "
				"does not have an IO-APIC object\n\tto manage "
				"it.\n",
				irqSourceEntry->destIoApicId);

			panic(ERROR_UNKNOWN);
		};

		isaBusPinMappings[irqSourceEntry->sourceBusIrq].__kpin =
			ioApic->get__kpinBase() + irqSourceEntry->destIoApicPin;

		isaBusPinMappings[irqSourceEntry->sourceBusIrq].isValid = 1;
		__kprintf(NOTICE IBMPCBPM"x86MP: Mapping ISA IRQ %d to __kpin "
			"%d (IO-APIC %d, pin %d).\n",
			irqSourceEntry->sourceBusIrq,
			isaBusPinMappings[irqSourceEntry->sourceBusIrq].__kpin,
			irqSourceEntry->destIoApicId,
			irqSourceEntry->destIoApicPin);
	};		 

	return ERROR_SUCCESS;
}

status_t ibmPc_bpm_isa_loadBusPinMappings(void)
{
	acpi_rMadtS		*rMadt;
	/**	EXPLANATION:
	 * This function fills out the array above, isaBusPinMappings[] with
	 * the most current information available on the mapping of ISA devices
	 * to the current IRQ subsystem in use.
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
		for (ubit8 i=0; i<16; i++)
		{
			if (ibmPc_irqControl_identifyIrq(
				i, &isaBusPinMappings[i].__kpin)
				!= ERROR_SUCCESS)
			{
				__kprintf(FATAL IBMPCBPM"Unable to get "
					"__kpinId for i8259 pin %d.\n",
					i);

				isaBusPinMappings[i].isValid = 0;
			}
			else {
				isaBusPinMappings[i].isValid = 1;
			};
		};

		return ERROR_SUCCESS;
	};

	// Else, use MADT and MP Tables to detect ISA IRQ mappings to IO-APICs.
	acpi::initializeCache();
	if (acpi::findRsdp() != ERROR_SUCCESS) {
		goto tryMpTables;
	};

#if !defined(__32_BIT__) || defined(CONFIG_ARCH_x86_32_PAE)
	if (acpi::testForXsdt())
	{
	};
#endif
	if (acpi::testForRsdt())
	{
		if (acpi::mapRsdt() != ERROR_SUCCESS)
		{
			__kprintf(NOTICE IBMPCBPM"Unable to map RSDT.\n");
			goto tryMpTables;
		};

		if (ibmPc_bpm_isa_acpi_checkRsdtForIsaMadtInfo(&rMadt))
		{
			ibmPc_bpm_isa_acpi_loadRsdtBusPinMappings(rMadt);
			acpiRsdt::destroySdt((acpi_sdtS *)rMadt);
			return ERROR_SUCCESS;
		};
	};

tryMpTables:
	x86Mp::initializeCache();
	if (x86Mp::findMpFp() == __KNULL)
	{
		__kprintf(FATAL IBMPCBPM"Anomally:\n"
			"\tYour chipset claims to support IO-APICs and SMP\n"
			"\tmode but did not supply ACPI IRQ Source Overrides,\n"
			"\tor even an MP Specification table.\n"
			"\tThe kernel cannot reliably determine where the ISA\n"
			"\tIRQs are mapped on the IO-APICs. Panicking.\n");

		panic(ERROR_UNKNOWN);
	};

	if (x86Mp::mapMpConfigTable() == __KNULL)
	{
		panic(ERROR IBMPCBPM"Failed to map MP Config table. There "
			"is a possibility that\n\tthere was bus->pin mapping "
			"info in there that was\n\tmissed. Halting.\n");
	};

	if (ibmPc_bpm_isa_x86Mp_loadBusPinMappings() != ERROR_SUCCESS)
	{
		panic(FATAL IBMPCBPM"Unable to read bus pin mappings from MP "
			"Tables.\n");
	};

	return ERROR_SUCCESS;
}	

error_t ibmPc_bpm_isa_get__kpinFor(ubit32 busIrqId, ubit16 *__kpin)
{
	// Just lookup the __kpin in the table.
	if (isaBusPinMappings[busIrqId].isValid)
	{
		*__kpin = isaBusPinMappings[busIrqId].__kpin;
		return ERROR_SUCCESS;
	};

	return ERROR_INVALID_ARG_VAL;
}

status_t ibmPc_bpm_isa_maskIrq(ubit32 busIrqId)
{
	ubit16		__kpin;
	error_t		ret;

	ret = ibmPc_bpm_isa_get__kpinFor(busIrqId, &__kpin);
	if (ret != ERROR_SUCCESS) {
		return ret;
	};

	ibmPc_irqControl_maskIrq(__kpin);
	return ERROR_SUCCESS;
}

status_t ibmPc_bpm_isa_unmaskIrq(ubit32 busIrqId)
{
	ubit16		__kpin;
	error_t		ret;

	ret = ibmPc_bpm_isa_get__kpinFor(busIrqId, &__kpin);
	if (ret != ERROR_SUCCESS) {
		return ret;
	};

	ibmPc_irqControl_unmaskIrq(__kpin);
	return ERROR_SUCCESS;
}

