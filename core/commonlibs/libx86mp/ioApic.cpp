
#include <arch/walkerPageRanger.h>
#include <__kclasses/debugPipe.h>
#include <commonlibs/libx86mp/ioApic.h>
#include <kernel/common/memoryTrib/memoryTrib.h>


x86IoApic::ioApicC::ioApicC(ubit8 id, paddr_t paddr)
:
paddr(paddr), id(id)
{
	vaddr.rsrc = __KNULL;
	nIrqs = 0;
}

x86IoApic::ioApicC::~ioApicC(void)
{
	unmapIoApic(vaddr.rsrc);
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
		__kprintf(ERROR x86IOAPIC"Failed to map APIC @0x%P to 0x%p.\n",
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
	// Map the IO-APIC into kernel vaddrspace.
	vaddr.rsrc = mapIoApic(paddr);
	if (vaddr.rsrc == __KNULL) { return ERROR_MEMORY_VIRTUAL_PAGEMAP; };

	// Get version.
	vaddr.lock.acquire();
	writeIoRegSel(x86IOAPIC_REG_VERSION);
	version = readIoWin();
	nIrqs = (readIoWin() >> 16) + 1;
	vaddr.lock.release();

	// Mask off all IRQs.
	__kprintf(NOTICE x86IOAPIC"%d: Masking off all IRQs for now.\n", id);
	for (ubit8 i=0; i<nIrqs; i++) {
		maskIrq(i);
	};

	__kprintf(NOTICE x86IOAPIC"%d: Initialize: v 0x%p, p 0x%P, ver 0x%x, "
		"nIrqs %d.\n",
		id, vaddr.rsrc, paddr, version, nIrqs);

	return ERROR_SUCCESS;
}

sarch_t x86IoApic::ioApicC::getIrqState(
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

void x86IoApic::ioApicC::setIrqState(
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

void x86IoApic::ioApicC::maskIrq(ubit8 irq)
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

void x86IoApic::ioApicC::unmaskIrq(ubit8 irq)
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

