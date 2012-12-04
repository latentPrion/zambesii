
#include <arch/walkerPageRanger.h>
#include <__kstdlib/__kclib/string.h>
#include <__kclasses/debugPipe.h>
#include <commonlibs/libx86mp/libx86mp.h>
#include <commonlibs/libx86mp/ioApic.h>
#include <commonlibs/libacpi/libacpi.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/interruptTrib/interruptTrib.h>


x86IoApic::ioApicC::ioApicC(ubit8 id, paddr_t paddr, sarch_t acpiGirqBase)
:
paddr(paddr), id(id), acpiGirqBase(acpiGirqBase)
{
	vaddr.rsrc = __KNULL;
	nIrqs = 0;
	__kpinBase = 0;
}

x86IoApic::ioApicC::~ioApicC(void)
{
	if (vaddr.rsrc != __KNULL)
		{ unmapIoApic(vaddr.rsrc); };

	vaddr.rsrc = __KNULL;

	if (irqPinList != __KNULL)
		{ delete [] irqPinList; };

	irqPinList = __KNULL;
}

x86IoApic::ioApicC::ioApicRegspaceS *x86IoApic::ioApicC::mapIoApic(
	paddr_t paddr
	)
{
	ioApicRegspaceS		*ret;
	status_t		status;

	/* Allocates a page and maps it to the paddr of an IO APIC.
	 **/
	// IO-APICs are always 1KB aligned.
	ret = (ioApicRegspaceS *)(memoryTrib.__kmemoryStream.vaddrSpaceStream
		.*memoryTrib.__kmemoryStream.vaddrSpaceStream.getPages)(1);

	if (ret == __KNULL) { return ret; };

	/* Now map the page to the paddr passed as an arg. It is worth noting
	 * that Linux maps the APICs as uncacheable.
	 **/
	status = walkerPageRanger::mapInc(
		&memoryTrib.__kmemoryStream.vaddrSpaceStream.vaddrSpace,
		ret, paddr, 1,
		PAGEATTRIB_PRESENT | PAGEATTRIB_WRITE
		| PAGEATTRIB_CACHE_WRITE_THROUGH | PAGEATTRIB_SUPERVISOR);

	if (status < 1)
	{
		__kprintf(ERROR x86IOAPIC"%d: Failed to map: v:0x%p, p:0x%P.\n",
			paddr, ret);

		memoryTrib.__kmemoryStream.vaddrSpaceStream.releasePages(
			ret, 1);

		return __KNULL;
	};

	ret = WPRANGER_ADJUST_VADDR(ret, paddr, ioApicRegspaceS *);
	return ret;
}

void x86IoApic::ioApicC::unmapIoApic(ioApicRegspaceS *ioApic)
{
	memoryTrib.__kmemoryStream.vaddrSpaceStream.releasePages(ioApic, 1);
}

/*
static zkcmIrqPinS		tmp;

static void sortListBy__kids(ubit8 nPins, zkcmIrqPinS *const list)
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

error_t x86IoApic::ioApicC::initialize(void)
{
	cpu_t		cpu=0;
	ubit8		vector, polarity, triggMode, dummy;

	// Map the IO-APIC into the kernel vaddrspace.
	vaddr.rsrc = mapIoApic(paddr);
	if (vaddr.rsrc == __KNULL) { return ERROR_MEMORY_VIRTUAL_PAGEMAP; };

	// Get version and number of pins.
	vaddr.lock.acquire(); 
	writeIoRegSel(x86IOAPIC_REG_VERSION);
	version = readIoWin() & 0xFF;
	nIrqs = ((readIoWin() >> 16) & 0xFF) + 1;
	vaddr.lock.release();

	// Allocate irqPinList.
	irqPinList = new zkcmIrqPinS[nIrqs];
	if (irqPinList == __KNULL)
	{
		__kprintf(ERROR x86IOAPIC"%d: Failed to allocate IRQ pin list."
			"\n", id);

		return ERROR_MEMORY_NOMEM;
	};

	/* Prepare and fill out the irqPinList. This includes filling out all
	 * ACPI global IRQ pin IDs.
	 **/
	for (ubit8 i=0; i<nIrqs; i++)
	{
		// Get current values.
		maskPin(i);
		getPinState(
			i, reinterpret_cast<ubit8 *>( &cpu ), &vector,
			&dummy, &dummy,
			&polarity, &triggMode);

		// Fill in ACPI Global IRQ ID and set Intel MP ID to invalid.
		irqPinList[i].intelMpId = IRQPIN_INTELMPID_INVALID;
		if (acpiGirqBase != IRQPIN_ACPIID_INVALID) {
			irqPinList[i].acpiId = acpiGirqBase + i;
		};

		irqPinList[i].triggerMode
			= (triggMode == x86IOAPIC_TRIGGMODE_EDGE) ?
				IRQPIN_TRIGGMODE_EDGE : IRQPIN_TRIGGMODE_LEVEL;

		irqPinList[i].polarity
			= (polarity == x86IOAPIC_POLARITY_LOW) ?
				IRQPIN_POLARITY_LOW : IRQPIN_POLARITY_HIGH;

		irqPinList[i].cpu = cpu;
		irqPinList[i].vector = vector;
		irqPinList[i].flags = 0;
	};

	// Give the pins to the kernel for __kpin ID assignment.
	interruptTrib.registerIrqPins(nIrqs, irqPinList);
	__kpinBase = irqPinList[0].__kid;

	__kprintf(NOTICE x86IOAPIC"%d: Initialize: v 0x%p, p 0x%P, ver 0x%x, "
		"nIrqs %d, Girqbase %d.\n\tAll pins masked off for now.\n",
		id, vaddr.rsrc, paddr, version, nIrqs, acpiGirqBase);

	// Now check to see if there are entries for each pin in the MP tables.
	getIntelMpPinMappings();
	return ERROR_SUCCESS;
}

sarch_t x86IoApic::ioApicC::getPinState(
	ubit8 irq, ubit8 *cpu, ubit8 *vector,
	ubit8 *deliveryMode, ubit8 *destMode,
	ubit8 *polarity, ubit8 *triggMode
	)
{
	ubit32		high, low;
	sarch_t		ret;

	// Not sure if this is the correct way to access the 64-bit regs.
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

	ret = !__KFLAG_TEST(low, x86IOAPIC_IRQSTATE_MASKED);
	return ret;
}

void x86IoApic::ioApicC::setPinState(
	ubit8 irq, ubit8 cpu, ubit8 vector,
	ubit8 deliveryMode, ubit8 destMode,
	ubit8 polarity, ubit8 triggMode
	)
{
	ubit32		high=0, low=0;

	high = x86IOAPIC_IRQTABLE_SETDEST(cpu);
	low |= x86IOAPIC_IRQTABLE_SETVECTOR(vector);
	low |= x86IOAPIC_IRQTABLE_SETDELIVERYMODE(deliveryMode);
	low |= x86IOAPIC_IRQTABLE_SETDESTMODE(destMode);
	low |= x86IOAPIC_IRQTABLE_SETPOLARITY(polarity);
	low |= x86IOAPIC_IRQTABLE_SETTRIGGMODE(triggMode);

	/* Same here, not sure if this is how the 64-bit regs are accessed.
	 * Also, of course, write the high 32 bits first.
	 **/
	vaddr.lock.acquire();
	writeIoRegSel(x86IOAPIC_REG_IRQTABLE(irq, 1));
	writeIoWin(high);
	writeIoRegSel(x86IOAPIC_REG_IRQTABLE(irq, 0));
	writeIoWin(low);
	vaddr.lock.release();
}

void x86IoApic::ioApicC::maskPin(ubit8 irq)
{
	ubit32		low;

	// Read low then set the mask and write it back.
	vaddr.lock.acquire();
	writeIoRegSel(x86IOAPIC_REG_IRQTABLE(irq, 0));
	low = readIoWin();
	__KFLAG_SET(low, x86IOAPIC_IRQSTATE_MASKED);
	writeIoWin(low);
	vaddr.lock.release();
}

void x86IoApic::ioApicC::unmaskPin(ubit8 irq)
{
	ubit32		low;

	// Read low, unset the mask and write it back.
	vaddr.lock.acquire();
	writeIoRegSel(x86IOAPIC_REG_IRQTABLE(irq, 0));
	low = readIoWin();
	__KFLAG_UNSET(low, x86IOAPIC_IRQSTATE_MASKED);
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

error_t x86IoApic::ioApicC::getIntelMpPinMappings(void)
{
	x86_mpCfgS		*mpCfg;
	x86_mpCfgIrqSourceS	*irqSource;
	uarch_t			context;
	void			*handle;
	ubit16			nPins=0;

	/**	EXPLANATION:
	 * Parse the MP tables and assign each pin its Intel MP ID.
	 **/
	x86Mp::initializeCache();
	if (x86Mp::findMpFp() == __KNULL)
	{
		__kprintf(WARNING x86IOAPIC"getIntelMpPinMappings: No MP.\n");
		return ERROR_GENERAL;
	};

	if ((mpCfg = x86Mp::mapMpConfigTable()) == __KNULL)
	{
		__kprintf(ERROR x86IOAPIC"getIntelMpPinMappings: Failed to "
			"map MP Configuration table.\n");

		return ERROR_MEMORY_NOMEM;
	};

	context = 0;
	handle = __KNULL;

	irqSource = x86Mp::getNextIrqSourceEntry(&context, &handle);
	for (; irqSource != __KNULL;
		irqSource = x86Mp::getNextIrqSourceEntry(&context, &handle))
	{
		// If not for this IO-APIC, skip it.
		if (irqSource->destIoApicId != id) { continue; };

		nPins++;
		// Print info for debug.
		/*__kprintf(NOTICE x86IOAPIC"getIntelMpPinMappings: "
			"bus-IRQ(%d, %d), IoApic(%d, %d),\n"
			"\tintType %s, Pol %s, trigg %s.\n",
			irqSource->sourceBusId, irqSource->sourceBusIrq,
			irqSource->destIoApicId, irqSource->destIoApicPin,
			intTypes[irqSource->intType],
			polarities[
				irqSource->flags
				& x86_MPCFG_IRQSRC_FLAGS_POLARITY_MASK],

			triggerModes[
				irqSource->flags
				>> x86_MPCFG_IRQSRC_FLAGS_SENSITIVITY_SHIFT
				& x86_MPCFG_IRQSRC_FLAGS_SENSITIVITY_MASK]); */

		/**	EXPLANATION:
		 * The only thing we do in here is check to see if the pin
		 * has an entry in the MP Tables. If it does, we set the
		 * intelMpId field to 0. Else, if there is no entry, we leave
		 * it at IRQPIN_INTELMPID_INVALID.
		 **/
		irqPinList[irqSource->destIoApicPin].intelMpId = 0;
	};

	__kprintf(NOTICE x86IOAPIC"getIntelMpPinMappings: %d pins.\n", nPins);
	return ERROR_SUCCESS;
}

