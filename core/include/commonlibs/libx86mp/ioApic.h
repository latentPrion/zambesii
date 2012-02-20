#ifndef _LIB_x86MP_IO_APIC_H
	#define _LIB_x86MP_IO_APIC_H

	#include <arch/paddr_t.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kflagManipulation.h>
	#include <__kclasses/hardwareIdList.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

#define x86IOAPIC				"IO-APIC "

#define x86IOAPIC_MAGIC				(0x10A1D1C0)

#define x86IOAPIC_REG_ID			0x0
#define x86IOAPIC_REG_VERSION			0x1
#define x86IOAPIC_REG_ARBID			0x2
#define x86IOAPIC_REG_IRQTABLE(__n,__o)		(0x10 + ((__n) << 1) + (__o))

#define x86IOAPIC_IRQTABLE_SETVECTOR(__n)	(__n & 0xFF)
#define x86IOAPIC_IRQTABLE_GETVECTOR(__l)	(__l & 0xFF)
#define x86IOAPIC_IRQTABLE_VECTORMASK		(0xFF)

#define x86IOAPIC_IRQTABLE_SETDELIVERYMODE(__m)	((__m & 0x7) << 8)
#define x86IOAPIC_IRQTABLE_GETDELIVERYMODE(__l)	((__l >> 8) & 0x7)
#define x86IOAPIC_IRQTABLE_DELIVERYMODEMASK	(0x7 << 8)
#define x86IOAPIC_DELIVERYMODE_FIXED		0x0
#define x86IOAPIC_DELIVERYMODE_LOWPRIO		0x1
#define x86IOAPIC_DELIVERYMODE_SMI		0x2
#define x86IOAPIC_DELIVERYMODE_NMI		0x4
#define x86IOAPIC_DELIVERYMODE_INIT		0x5
#define x86IOAPIC_DELIVERYMODE_EXTINT		0x7

#define x86IOAPIC_IRQTABLE_SETDESTMODE(__m)	(__m << 11)
#define x86IOAPIC_IRQTABLE_GETDESTMODE(__l)	((__l >> 11) & 0x1)
#define x86IOAPIC_IRQTABLE_DESTMODEMASK		(0x1 << 11)
#define x86IOAPIC_DESTMODE_PHYSICAL		0x0
#define x86IOAPIC_DESTMODE_LOGICAL		0x1

#define x86IOAPIC_IRQTABLE_GETDELIVERYSTAT(__l)	(__l >> 12)

#define x86IOAPIC_IRQTABLE_SETPOLARITY(__p)	(__p << 13)
#define x86IOAPIC_IRQTABLE_GETPOLARITY(__l)	((__l >> 13) & 0x1)
#define x86IOAPIC_IRQTABLE_POLARITYMASK		(0x1 << 13)
#define x86IOAPIC_POLARITY_HIGH			0x0
#define x86IOAPIC_POLARITY_LOW			0x1

#define x86IOAPIC_IRQTABLE_GETIRR(__l)		((__l >> 14) & 0x1)

#define x86IOAPIC_IRQTABLE_SETTRIGGMODE(__m)	(__m << 15)
#define x86IOAPIC_IRQTABLE_GETTRIGGMODE(__l)	((__l >> 15) & 0x1)
#define x86IOAPIC_IRQTABLE_TRIGGMODEMASK	(0x1 << 15)
#define x86IOAPIC_TRIGGMODE_EDGE		0x0
#define x86IOAPIC_TRIGGMODE_LEVEL		0x1

// Just set with __KFLAG_SET()/__KFLAG_UNSET().
#define x86IOAPIC_IRQSTATE_MASKED		(1<<16)

#define x86IOAPIC_IRQTABLE_SETDEST(__d)		(__d << 24)
#define x86IOAPIC_IRQTABLE_GETDEST(__h)		((__h >> 24) & 0xFF)
#define x86IOAPIC_IRQTABLE_DESTMASK		(0xFF << 24)

namespace x86IoApic
{
	class ioApicC
	{
	public:
		ioApicC(ubit8 id, paddr_t paddr);
		// Gets version, etc, masks every IRQ on the particular IO-APIC.
		error_t initialize(void);
		~ioApicC(void);

	public:
		ubit8 getNIrqs(void);
		void setIrqState(
			ubit8 irq, ubit8 cpu, ubit8 vector,
			ubit8 deliveryMode, ubit8 destMode,
			ubit8 polarity, ubit8 triggMode);

		// Returns 1 if unmasked, 0 if masked off.
		sarch_t getIrqState(
			ubit8 irq, ubit8 *cpu, ubit8 *vector,
			ubit8 *deliveryMode, ubit8 *destMode,
			ubit8 *polarity, ubit8 *triggMode);

		void maskIrq(ubit8 irq);
		void unmaskIrq(ubit8 irq);

	private:
		inline void writeIoRegSel(ubit8 val);
		inline ubit32 readIoWin(void);
		inline void readIoWin(ubit32 *high, ubit32 *low);
		inline void writeIoWin(ubit32 val);
		inline void writeIoWin(ubit32 high, ubit32 low);

		struct ioApicRegspaceS;
		ioApicRegspaceS *mapIoApic(paddr_t paddr);
		void unmapIoApic(ioApicRegspaceS *vaddr);

	private:
		struct ioApicRegspaceS
		{
			// All writes to this reg must be 32-bit.
			ubit32			ioRegSel[4];
			// All reads and writes to/from this reg must be 32-bit.
			volatile ubit32		ioWin[4];
		};

		paddr_t			paddr;
		ubit8			id, version, nIrqs;
		sharedResourceGroupC<waitLockC, ioApicRegspaceS *>	vaddr;
	};

	struct cacheS
	{
		ubit32			magic;
		sarch_t			mapped;
		ubit8			nIoApics;

		// List of IO APIC objects.
		hardwareIdListC		ioApics;
	};

	void initializeCache(void);
	void flushCache(void);
	void dump(void);

	sarch_t ioApicsAreDetected(void);
	error_t detectIoApics(void);
	ubit16 getNIoApics(void);

	ioApicC *getIoApic(ubit8 id);
}


/**	Inline methods
 *****************************************************************************/

void x86IoApic::ioApicC::writeIoRegSel(ubit8 val)
{
	vaddr.rsrc->ioRegSel[0] = val;
	asm volatile ("":::"memory");
}

ubit32 x86IoApic::ioApicC::readIoWin(void)
{
	return vaddr.rsrc->ioWin[0];
}

void x86IoApic::ioApicC::readIoWin(ubit32 *high, ubit32 *low)
{
	*high = vaddr.rsrc->ioWin[1];
	*low = vaddr.rsrc->ioWin[0];
}

void x86IoApic::ioApicC::writeIoWin(ubit32 val)
{
	vaddr.rsrc->ioWin[0] = val;
	asm volatile("":::"memory");
}

void x86IoApic::ioApicC::writeIoWin(ubit32 high, ubit32 low)
{
	vaddr.rsrc->ioWin[1] = high;
	vaddr.rsrc->ioWin[0] = low;
}

#endif

