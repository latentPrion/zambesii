
#include <__kstdlib/__ktypes.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kclasses/debugPipe.h>
#include <commonlibs/libacpi/libacpi.h>
#include <commonlibs/libx86mp/libx86mp.h>
#include <kernel/common/panic.h>

#include "zkcmIbmPcState.h"
#include "irqControl.h"

#define IBMPCBPM		"BusPinMapping: "

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

	handle = __KNULL;
	overrideEntry = acpiRMadt::getNextIrqSourceOverrideEntry(
		madt, &handle);

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
		__kprintf(NOTICE IBMPCBPM"ACPI ISA Override: mapping ISA IRQ "
			"%d to __kpin %d (girq is %d).\n",
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

static status_t ibmPc_bpm_isa_loadBusPinMappings(void)
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

	/* Else, use MADT and MP Tables to detect ISA IRQ mappings to IO-APICs.
	 * First, assume all of them are identity mapped. Then look for the
	 * override structure and see if there is any need for remapping.
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
		/* See the final return statement below to see why we return
		 * ERROR_SUCCESS here.
		 **/
		return ERROR_SUCCESS;
	};

	if (x86Mp::mapMpConfigTable() == __KNULL)
	{
		panic(ERROR IBMPCBPM"Failed to map MP Config table. There "
			"is a possibility that\n\tthere was bus->pin mapping "
			"info in there that we\n\tmissed. Halting.\n");
	};

	if (ibmPc_bpm_isa_x86Mp_loadBusPinMappings() != ERROR_SUCCESS)
	{
		panic(FATAL IBMPCBPM"Unable to read bus pin mappings from MP "
			"Tables.\n");
	};

	/**	NOTE:
	 * ISA IRQ mapping detection cannot return IRQCTL_BPM_NO_MAPPINGS_FOUND
	 * because by default we have to assume that if we find no extra
	 * mapping information for ISA IRQs, then all ISA IRQs are mapped to
	 * to their default pin inputs on either the i8259 or the IO-APICs,
	 * depending on which IRQ routing subsystem is currently active.
	 **/
	return ERROR_SUCCESS;
}	

status_t ibmPc_irqControl_bpm_loadBusPinMappings(utf8Char *bus)
{
	if (strcmp8(bus, CC"isa") == 0) {
		return ibmPc_bpm_isa_loadBusPinMappings();
	};

	// The ACPI BPM function will generally find out where SCI is mapped.
	if (strcmp8(bus, CC"pci") == 0 || strcmp8(bus, CC"acpi") == 0)
	{
		__kprintf(FATAL IBMPCBPM"PCI and ACPI buses are not yet "
			"supported.\n");

		return IRQCTRL_BPM_UNSUPPORTED_BUS;
	};

	return IRQCTRL_BPM_UNSUPPORTED_BUS;
}


