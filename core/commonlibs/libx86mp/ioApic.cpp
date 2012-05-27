
#include <arch/walkerPageRanger.h>
#include <__kclasses/debugPipe.h>
#include <commonlibs/libx86mp/ioApic.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/interruptTrib/interruptTrib.h>

#include <debug.h>
	

x86IoApic::ioApicC::ioApicC(ubit8 id, paddr_t paddr)
:
paddr(paddr), id(id)
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

error_t x86IoApic::ioApicC::initialize(void)
{
	cpu_t		cpu;
	ubit8		vector, polarity, triggMode, dummy;
	sarch_t		enabled;

	// Map the IO-APIC into the kernel vaddrspace.
	vaddr.rsrc = mapIoApic(paddr);
	if (vaddr.rsrc == __KNULL) { return ERROR_MEMORY_VIRTUAL_PAGEMAP; };

	// Get version.
	vaddr.lock.acquire(); 
	writeIoRegSel(x86IOAPIC_REG_VERSION);
	version = readIoWin();
	nIrqs = (readIoWin() >> 16) + 1;
	vaddr.lock.release();

	// Allocate irqPinList.
	irqPinList = new zkcmIrqPinS[nIrqs];
	if (irqPinList == __KNULL)
	{
		__kprintf(ERROR x86IOAPIC"%d: Failed to allocate IRQ pin list."
			"\n", id);

		return ERROR_MEMORY_NOMEM;
	};

	// Mask off all IRQs.
	__kprintf(NOTICE x86IOAPIC"%d: Masking off all IRQs for now.\n", id);
	for (ubit8 i=0; i<nIrqs; i++)
		{ maskIrq(i); };

	// Prepare and fill out the irqPinList.
	for (ubit8 i=0; i<nIrqs; i++)
	{
		enabled = getPinState(
			i, reinterpret_cast<ubit8 *>( &cpu ), &vector,
			&dummy, &dummy,
			&polarity, &triggMode);

		
		irqPinList[i].acpiId = 0;
		irqPinList[i].flags = 0;

		irqPinList[i].cpu = cpu;
		irqPinList[i].vector = vector;
		irqPinList[i].triggerMode = triggMode;
		irqPinList[i].polarity = polarity;
	};

	// Give the pins to the kernel for __kpin ID assignment.
	interruptTrib.registerIrqPins(nIrqs, irqPinList);
	__kpinBase = irqPinList[0].__kid;

	__kprintf(NOTICE x86IOAPIC"%d: Initialize: v 0x%p, p 0x%P, ver 0x%x, "
		"nIrqs %d.\n",
		id, vaddr.rsrc, paddr, version, nIrqs);

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

error_t x86IoApic::ioApicC::identifyIrq(uarch_t physicalId, ubit16 *__kpin)
{
	/**	EXPLANATION:
	 * We assume that any caller of this function is calling to ask for
	 * a lookup based on an ACPI ID. So we check to see if any of the pins
	 * on this IO-APIC match, and if not, we return an error.
	 **/
	for (ubit8 i=0; i<nIrqs; i++)
	{
		if (irqPinList[i].acpiId == physicalId)
		{
			*__kpin = irqPinList[i].__kid;
			return ERROR_SUCCESS;
		};
	};

	return ERROR_INVALID_ARG_VAL;
}

status_t x86IoApic::ioApicC::setIrqStatus(
	uarch_t __kpin, cpu_t cpu, uarch_t vector, ubit8 enabled
	)
{
	error_t		err;
	ubit8		pin;
	ubit8		dummy, destMode, deliveryMode, triggerMode, polarity;
	sarch_t		isEnabled;

	err = lookupPinBy__kid(__kpin, &pin);
	if (err != ERROR_SUCCESS) { return IRQCTL_IRQSTATUS_INEXISTENT; };

	isEnabled = getPinState(
		pin, &dummy, &dummy,
		&deliveryMode, &destMode, &polarity, &triggerMode);

	setPinState(
		pin, cpu, vector,
		deliveryMode, destMode, polarity, triggerMode);

	if (enabled && (!isEnabled))
	{
		unmaskPin(pin);
		__KFLAG_SET(irqPinList[pin].flags, IRQPIN_FLAGS_ENABLED);
	};

	if ((!enabled) && isEnabled)
	{
		maskPin(pin);
		__KFLAG_UNSET(irqPinList[pin].flags, IRQPIN_FLAGS_ENABLED);
	};

	irqPinList[pin].cpu = cpu;
	irqPinList[pin].vector = vector;

	return ERROR_SUCCESS;
}

status_t x86IoApic::ioApicC::getIrqStatus(
	uarch_t __kpin, cpu_t *cpu, uarch_t *vector,
	ubit8 *triggerMode, ubit8 *polarity
	)
{
	error_t		err;
	ubit8		pin;

	err = lookupPinBy__kid(__kpin, &pin);
	if (err != ERROR_SUCCESS) { return IRQCTL_IRQSTATUS_INEXISTENT; };

	*cpu = irqPinList[pin].cpu;
	*vector = irqPinList[pin].vector;
	*triggerMode = irqPinList[pin].triggerMode;
	*polarity = irqPinList[pin].triggerMode;

	return __KFLAG_TEST(irqPinList[pin].flags, IRQPIN_FLAGS_ENABLED) ?
		IRQCTL_IRQSTATUS_ENABLED : IRQCTL_IRQSTATUS_DISABLED;
}

void x86IoApic::ioApicC::maskIrq(ubit16 __kpin)
{
	error_t		err;
	ubit8		pin;

	err = lookupPinBy__kid(__kpin, &pin);
	if (err != ERROR_SUCCESS) { return; };

	maskPin(pin);
	__KFLAG_UNSET(irqPinList[pin].flags, IRQPIN_FLAGS_ENABLED);
}

void x86IoApic::ioApicC::unmaskIrq(ubit16 __kpin)
{
	error_t		err;
	ubit8		pin;

	err = lookupPinBy__kid(__kpin, &pin);
	if (err != ERROR_SUCCESS) { return; };

	unmaskPin(pin);
	__KFLAG_SET(irqPinList[pin].flags, IRQPIN_FLAGS_ENABLED);
}

sarch_t x86IoApic::ioApicC::irqIsEnabled(ubit16 __kpin)
{
	error_t		err;
	ubit8		pin;

	err = lookupPinBy__kid(__kpin, &pin);
	if (err != ERROR_SUCCESS) { return 0; };

	return __KFLAG_TEST(irqPinList[pin].flags, IRQPIN_FLAGS_ENABLED);
}

void x86IoApic::ioApicC::maskAll(void)
{
	for (ubit8 i=0; i<nIrqs; i++)
	{
		maskPin(i);
		__KFLAG_UNSET(irqPinList[i].flags, IRQPIN_FLAGS_ENABLED);
	}
}

void x86IoApic::ioApicC::unmaskAll(void)
{
	for (ubit8 i=0; i<nIrqs; i++)
	{
		unmaskPin(i);
		__KFLAG_SET(irqPinList[i].flags, IRQPIN_FLAGS_ENABLED);
	}
}

