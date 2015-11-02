
#include <arch/walkerPageRanger.h>
#include <__kstdlib/__kclib/string.h>
#include <__kclasses/debugPipe.h>
#include <commonlibs/libx86mp/libx86mp.h>
#include <commonlibs/libx86mp/ioApic.h>
#include <commonlibs/libacpi/libacpi.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/interruptTrib/interruptTrib.h>


x86IoApic::IoApic::~IoApic(void)
{
	if (vaddr.rsrc != NULL)
		{ unmapIoApic(vaddr.rsrc); };

	vaddr.rsrc = NULL;

	if (irqPinList != NULL)
		{ delete [] irqPinList; };

	irqPinList = NULL;
}

x86IoApic::IoApic::sIoApicRegspace *x86IoApic::IoApic::mapIoApic(
	paddr_t paddr
	)
{
	sIoApicRegspace		*ret;
	status_t		status;

	/* Allocates a page and maps it to the paddr of an IO APIC.
	 **/
	// IO-APICs are always 1KB aligned.
	ret = (sIoApicRegspace *)processTrib.__kgetStream()
		->getVaddrSpaceStream()->getPages(1);

	if (ret == NULL) { return ret; };

	/* Now map the page to the paddr passed as an arg. It is worth noting
	 * that Linux maps the APICs as uncacheable.
	 **/
	status = walkerPageRanger::mapInc(
		&processTrib.__kgetStream()->getVaddrSpaceStream()->vaddrSpace,
		ret, paddr, 1,
		PAGEATTRIB_PRESENT | PAGEATTRIB_WRITE
		| PAGEATTRIB_CACHE_DISABLED | PAGEATTRIB_SUPERVISOR);

	if (status < 1)
	{
		printf(ERROR x86IOAPIC"%d: Failed to map: v:0x%p, p:0x%P.\n",
			id, ret, &paddr);

		processTrib.__kgetStream()->getVaddrSpaceStream()->releasePages(
			ret, 1);

		return NULL;
	};

	ret = WPRANGER_ADJUST_VADDR(ret, paddr, sIoApicRegspace *);
	return ret;
}

void x86IoApic::IoApic::unmapIoApic(sIoApicRegspace *ioApic)
{
	processTrib.__kgetStream()->getVaddrSpaceStream()->releasePages(ioApic, 1);
}

/*
static sZkcmIrqPin		tmp;

static void sortListBy__kids(ubit8 nPins, sZkcmIrqPin *const list)
{
	for (ubit8 i=0; i<nPins-1; )
	{
		if (list[i].__kid > list[i + 1].__kid)
		{
			memcpy(&tmp, &list[i], sizeof(*list));
			memcpy(&list[i], &list[i+1], sizeof(*list));
			memcpy(&list[i+1], &tmp, sizeof(*list));

			if (i != 0) { i--; };
		}
		else { i++; };
	};
}
*/

error_t x86IoApic::IoApic::initialize(void)
{
	error_t		err;

	// Map the IO-APIC into the kernel vaddrspace.
	vaddr.rsrc = mapIoApic(paddr);
	if (vaddr.rsrc == NULL) { return ERROR_MEMORY_VIRTUAL_PAGEMAP; };

	// Get version and number of pins.
	vaddr.lock.acquire();
	writeIoRegSel(x86IOAPIC_REG_VERSION);
	version = readIoWin() & 0xFF;
	nPins = ((readIoWin() >> 16) & 0xFF) + 1;
	vaddr.lock.release();

	// Get vector base. See ZkcmIrqControlMod::identifyActiveIrq().
	err = x86IoApic::allocateVectorBaseFor(this, &vectorBase);
	if (err != ERROR_SUCCESS) { return err; };

	// Allocate irqPinList.
	irqPinList = new sZkcmIrqPin[nPins];
	if (irqPinList == NULL)
	{
		printf(ERROR x86IOAPIC"%d: Failed to allocate IRQ pin list."
			"\n", id);

		return ERROR_MEMORY_NOMEM;
	};

	/* Prepare and fill out the irqPinList. This includes filling out all
	 * ACPI global IRQ pin IDs.
	 **/
	for (ubit8 i=0; i<nPins; i++)
	{
		/**	EXPLANATION:
		 * For the IO-APICs, the kernel mandates the following state
		 * upon detection and initialization: all pins must be masked
		 * off.
		 *
		 * The exact programming status of any individual pin with
		 * reference to target CPU, vector, polarity, trigger mode,
		 * destination mode, delivery mode, etc are all undefined.
		 *
		 * These details are dealt with by the bus-pin mapping code when
		 * a bus is attached to the kernel. For example, when the ISA
		 * bus is attached and its bus-pin mappings are loaded
		 * (loadBusPinMappings(CC"isa"), the bus-pin mapping code
		 * will at that time program all the ISA pins correctly as
		 * needed for the ISA bus, and as directed by the ACPI/MP
		 * tables.
		 **/
		maskPin(i);

		// Set the vector and __kpinID for each pin.
		irqPinList[i].vector = vectorBase + i;

		// Fill in ACPI Global IRQ ID and set Intel MP ID to invalid.
		irqPinList[i].intelMpId = IRQCTL_IRQPIN_INTELMPID_INVALID;
		if (acpiGirqBase != IRQCTL_IRQPIN_ACPIID_INVALID) {
			irqPinList[i].acpiId = acpiGirqBase + i;
		};
	};

	// Give the pins to the kernel for __kpin ID assignment.
	interruptTrib.registerIrqPins(nPins, irqPinList);
	__kpinBase = irqPinList[0].__kid;

	printf(NOTICE x86IOAPIC"%d: Initialize: v 0x%p, p 0x%P, ver 0x%x,\n"
		"\tnPins %d, Girqbase %d, vectorBase %d.\n",
		id, vaddr.rsrc, &paddr, version, nPins,
		acpiGirqBase, vectorBase);

	return ERROR_SUCCESS;
}

sarch_t x86IoApic::IoApic::getPinState(
	ubit8 irq, ubit8 *cpu, ubit8 *vector,
	ubit8 *deliveryMode, ubit8 *destMode,
	ubit8 *polarity, ubit8 *triggMode
	)
{
	ubit32		high, low;
	sarch_t		ret;

	vaddr.lock.acquire();
	writeIoRegSel(x86IOAPIC_REG_IRQTABLE(irq, 0));
	low = readIoWin();
	writeIoRegSel(x86IOAPIC_REG_IRQTABLE(irq, 1));
	high = readIoWin();
	vaddr.lock.release();

	*cpu = x86IOAPIC_IRQTABLE_GETDEST(high);
	*vector = x86IOAPIC_IRQTABLE_GETVECTOR(low);
	*deliveryMode = x86IOAPIC_IRQTABLE_GETDELIVERYMODE(low);
	*destMode = x86IOAPIC_IRQTABLE_GETDESTMODE(low);
	*polarity = x86IOAPIC_IRQTABLE_GETPOLARITY(low);
	*triggMode = x86IOAPIC_IRQTABLE_GETTRIGGMODE(low);

	ret = !FLAG_TEST(low, x86IOAPIC_IRQSTATE_MASKED);
	return ret;
}

void x86IoApic::IoApic::setPinState(
	ubit8 irq, ubit8 cpu, ubit8 vector,
	ubit8 deliveryMode, ubit8 destMode,
	ubit8 polarity, ubit8 triggMode
	)
{
	ubit32		high=0, low;

	// Must preserve the masking state of the pin (masked/unmasked).
	vaddr.lock.acquire();
	writeIoRegSel(x86IOAPIC_REG_IRQTABLE(irq, 0));
	low = readIoWin();
	vaddr.lock.release();

	low &= x86IOAPIC_IRQSTATE_MASKED;

	// Set the parameters.
	high = x86IOAPIC_IRQTABLE_SETDEST(cpu);
	low |= x86IOAPIC_IRQTABLE_SETVECTOR(vector);
	low |= x86IOAPIC_IRQTABLE_SETDELIVERYMODE(deliveryMode);
	low |= x86IOAPIC_IRQTABLE_SETDESTMODE(destMode);
	low |= x86IOAPIC_IRQTABLE_SETPOLARITY(polarity);
	low |= x86IOAPIC_IRQTABLE_SETTRIGGMODE(triggMode);

	vaddr.lock.acquire();
	writeIoRegSel(x86IOAPIC_REG_IRQTABLE(irq, 1));
	writeIoWin(high);
	writeIoRegSel(x86IOAPIC_REG_IRQTABLE(irq, 0));
	writeIoWin(low);
	vaddr.lock.release();
}

void x86IoApic::IoApic::maskPin(ubit8 irq)
{
	ubit32		low;

	// Read low then set the mask and write it back.
	vaddr.lock.acquire();
	writeIoRegSel(x86IOAPIC_REG_IRQTABLE(irq, 0));
	low = readIoWin();
	FLAG_SET(low, x86IOAPIC_IRQSTATE_MASKED);
	writeIoWin(low);
	vaddr.lock.release();
}

void x86IoApic::IoApic::unmaskPin(ubit8 irq)
{
	ubit32		low;

	// Read low, unset the mask and write it back.
	vaddr.lock.acquire();
	writeIoRegSel(x86IOAPIC_REG_IRQTABLE(irq, 0));
	low = readIoWin();
	FLAG_UNSET(low, x86IOAPIC_IRQSTATE_MASKED);
	writeIoWin(low);
	vaddr.lock.release();
}

/*static utf8Char		*intTypes[4] = {
	CC"INT", CC"NMI", CC"SMI", CC"Ext-INT"
};

static utf8Char		*polarities[4] = {
	CC"Conforms", CC"High", CC"Invalid", CC"Low"
};

static utf8Char		*triggerModes[4] = {
	CC"Conforms", CC"Edge", CC"Invalid", CC"Level"
};*/

