#ifndef _LIB_x86MP_IO_APIC_H
	#define _LIB_x86MP_IO_APIC_H

	#include <arch/interrupts.h>
	#include <arch/paddr_t.h>
	#include <chipset/zkcm/picDevice.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kflagManipulation.h>
	#include <__kstdlib/__kclib/assert.h>
	#include <__kclasses/hardwareIdList.h>
	#include <__kclasses/debugPipe.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>
	#include <kernel/common/panic.h>

#define x86IOAPIC				"IO-APIC "

#define x86IOAPIC_MAGIC				(0x10A1D1C0)

#define x86IOAPIC_MAX_NIOAPICS			24

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

// Just set with FLAG_SET()/FLAG_UNSET().
#define x86IOAPIC_IRQSTATE_MASKED		(1<<16)

#define x86IOAPIC_IRQTABLE_SETDEST(__d)		(__d << 24)
#define x86IOAPIC_IRQTABLE_GETDEST(__h)		((__h >> 24) & 0xFF)
#define x86IOAPIC_IRQTABLE_DESTMASK		(0xFF << 24)

namespace x86IoApic
{
	class IoApic
	:
	public ZkcmPicDevice
	{
	/**	EXPLANATION:
	 * Class IoApic is a device driver which can be instantiated, and
	 * it provides the ZKCM PIC driver interface to the kernel.
	 **/
	friend error_t ibmPc_isaBpm_smpMode_x86Mp_loadBusPinMappings(void);
	friend error_t ibmPc_isaBpm_smpMode_rsdt_loadBusPinMappings(void);
	public:
		IoApic(ubit8 id, paddr_t paddr, sarch_t acpiGirqBase)
		:
		// Set nPins to 0 until we know how many there are.
		ZkcmPicDevice(0, &baseDeviceInfo),
		// Set "childId" in ZkcmDevice to the IO-APIC ID.
		baseDeviceInfo(
			id, CC"IO-APIC", CC"Intel MP Compliant IO-APIC chip",
			CC"Unknown vendor", CC"N/A"),
		paddr(paddr), id(id), acpiGirqBase(acpiGirqBase), vectorBase(0)
		{
			vaddr.rsrc = NULL;
			version = 0;
		}

		// Gets version, etc, masks every IRQ on the particular IO-APIC.
		virtual error_t initialize(void);
		virtual error_t shutdown(void) { return ERROR_SUCCESS; };
		virtual error_t suspend(void) { return ERROR_SUCCESS; };
		virtual error_t restore(void) { return ERROR_SUCCESS; };
		virtual ~IoApic(void);

	public:
		ubit8 getNIrqs(void) { return nPins; }
		sarch_t getGirqBase(void) { return acpiGirqBase; }
		ubit8 getId(void) { return id; }
		ubit8 getVectorBase(void) { return vectorBase; }

		void maskPin(ubit8 irq);
		void unmaskPin(ubit8 irq);
		void setPinState(
			ubit8 irq, ubit8 cpu, ubit8 vector,
			ubit8 deliveryMode, ubit8 destMode,
			ubit8 polarity, ubit8 triggMode);

		// Returns 1 if unmasked, 0 if masked off.
		sarch_t getPinState(
			ubit8 irq, ubit8 *cpu, ubit8 *vector,
			ubit8 *deliveryMode, ubit8 *destMode,
			ubit8 *polarity, ubit8 *triggMode);

		virtual status_t identifyActiveIrq(
			cpu_t, uarch_t vector,
			ubit16 *__kpin, ubit8 *triggerMode)
		{
			// We just map the vector value to a __kpin value.
			*__kpin = irqPinList[vector - vectorBase].__kid;
			*triggerMode =
				irqPinList[vector - vectorBase].triggerMode;

			return ERROR_SUCCESS;
		}

		// Returns the registered __kpin ID for Girq ID on this IO-APIC.
		virtual error_t get__kpinFor(uarch_t physicalId, ubit16 *__kpin);
		inline error_t lookupPinBy__kid(ubit16 __kpin, ubit8 *pin);

		virtual status_t getIrqStatus(
			uarch_t __kpin, cpu_t *cpu, uarch_t *vector,
			ubit8 *triggerMode, ubit8 *polarity);

		virtual status_t setIrqStatus(
			uarch_t __kpin, cpu_t cpu, uarch_t vector,
			ubit8 enabled);

		virtual void maskIrq(ubit16 __kpin);
		virtual void unmaskIrq(ubit16 __kpin);
		virtual void maskAll(void);
		virtual void unmaskAll(void);
		virtual sarch_t irqIsEnabled(ubit16 __kpin);

		virtual void maskIrqsByPriority(
			ubit16, cpu_t, uarch_t *)
		{UNIMPLEMENTED("ioApic::IoApic::maskIrqsByPriority");}

		virtual void unmaskIrqsByPriority(
			ubit16, cpu_t, uarch_t)
		{UNIMPLEMENTED("ioApic::IoApic::unmaskIrqsByPriority");}

		// This function should never be called.
		virtual void sendEoi(ubit16)
		{UNIMPLEMENTED("ioApic::IoApic::sendEoi");}


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
		ZkcmDevice		baseDeviceInfo;
		struct ioApicRegspaceS
		{
			// All writes to this reg must be 32-bit.
			ubit32			ioRegSel[4];
			// All reads and writes to/from this reg must be 32-bit.
			volatile ubit32		ioWin[4];
		};

		paddr_t			paddr;
		ubit8			id, version;
		/* This is signed so that in the event that we are not on an
		 * ACPI supporting chipset, we can set it to a negative
		 * value to indicate that it is not valid.
		 **/
		sarch_t			acpiGirqBase;
		SharedResourceGroup<WaitLock, ioApicRegspaceS *>	vaddr;
		ubit8			vectorBase;
	};

	struct sCache
	{
		ubit32			magic;
		sarch_t			mapped;
		ubit8			nIoApics;

		// List of IO APIC objects.
		HardwareIdList		ioApics;

		/* The counter used to allocate new vector bases. Initial value
		 * hardcoded and set in flushCache().
		 **/
		ubit8		vectorBaseCounter;
	};

	void initializeCache(void);
	void flushCache(void);
	void dump(void);

	sarch_t ioApicsAreDetected(void);
	error_t detectIoApics(void);
	ubit16 getNIoApics(void);

	IoApic *getIoApic(ubit8 id);
	IoApic *getIoApicFor(ubit16 __kpin);
	// Looks up an ACPI Girq number and returns its registered __kpin ID.
	error_t get__kpinFor(uarch_t girqId, ubit16 *__kpin);

	inline status_t getIrqStatus(
		uarch_t __kpin, cpu_t *cpu, uarch_t *vector,
		ubit8 *triggerMode, ubit8 *polarity);

	inline status_t setIrqStatus(
		uarch_t __kpin, cpu_t cpu, uarch_t vector, ubit8 enabled);

	inline void maskIrq(ubit16 __kpin);
	inline void unmaskIrq(ubit16 __kpin);
	void maskAll(void);
	void unmaskAll(void);
	inline sarch_t irqIsEnabled(ubit16 __kpin);

	// void maskIrqsByPriority(ubit16 __kpin, cpu_t cpuId, uarch_t *mask0);
	// void unmaskIrqsByPriority(ubit16 __kpin, cpu_t cpuId, uarch_t mask0);

	error_t allocateVectorBaseFor(IoApic *ioApic, ubit8 *ret);
	IoApic *getIoApicByVector(ubit8 vector);
}


/**	Inline methods
 *****************************************************************************/

void x86IoApic::IoApic::writeIoRegSel(ubit8 val)
{
	assert_fatal(!((val > 0x2 && val < 0x10) || val > 0x3f));
	vaddr.rsrc->ioRegSel[0] = val;
	asm volatile ("":::"memory");
}

ubit32 x86IoApic::IoApic::readIoWin(void)
{
	return vaddr.rsrc->ioWin[0];
}

void x86IoApic::IoApic::readIoWin(ubit32 *high, ubit32 *low)
{
	*high = vaddr.rsrc->ioWin[1];
	*low = vaddr.rsrc->ioWin[0];
}

void x86IoApic::IoApic::writeIoWin(ubit32 val)
{
	vaddr.rsrc->ioWin[0] = val;
	asm volatile("":::"memory");
}

void x86IoApic::IoApic::writeIoWin(ubit32 high, ubit32 low)
{
	vaddr.rsrc->ioWin[1] = high;
	vaddr.rsrc->ioWin[0] = low;
}

error_t x86IoApic::IoApic::lookupPinBy__kid(ubit16 __kid, ubit8 *pin)
{
	*pin = __kid - __kpinBase;
	if (*pin >= nPins) { return ERROR_INVALID_ARG_VAL; };
	return ERROR_SUCCESS;
}

sarch_t x86IoApic::irqIsEnabled(ubit16 __kpin)
{
	IoApic		*ioApic;

	ioApic = getIoApicFor(__kpin);
	if (ioApic != NULL) {
		return ioApic->irqIsEnabled(__kpin);
	};

	printf(ERROR x86IOAPIC"LibIoApic relay: irqIsEnabled: __kpin %d "
		"doesn't map to any known IO-APIC.\n",
		__kpin);

	return 0;
}

status_t x86IoApic::getIrqStatus(
	uarch_t __kpin, cpu_t *cpu, uarch_t *vector,
	ubit8 *triggerMode, ubit8 *polarity
	)
{
	IoApic		*ioApic;

	ioApic = getIoApicFor(__kpin);
	if (ioApic != NULL)
	{
		return ioApic->getIrqStatus(
			__kpin, cpu, vector, triggerMode, polarity);
	};

	printf(ERROR x86IOAPIC"LibIoApic relay: getIrqStatus: __kpin %d "
		"doesn't map to any known IO-APIC.\n",
		__kpin);

	return IRQCTL_GETIRQSTATUS_INEXISTENT;
}

status_t x86IoApic::setIrqStatus(
	uarch_t __kpin, cpu_t cpu, uarch_t vector, ubit8 enabled
	)
{
	IoApic		*ioApic;

	ioApic = getIoApicFor(__kpin);
	if (ioApic != NULL) {
		return ioApic->setIrqStatus(__kpin, cpu, vector, enabled);
	};

	printf(ERROR x86IOAPIC"LibIoApic relay: setIrqStatus: __kpin %d "
		"doesn't map to any known IO-APIC.\n",
		__kpin);

	return ERROR_INVALID_ARG_VAL;
}

void x86IoApic::maskIrq(ubit16 __kpin)
{
	IoApic		*ioApic;

	ioApic = getIoApicFor(__kpin);
	if (ioApic != NULL)
	{
		ioApic->maskIrq(__kpin);
		return;
	};

	printf(ERROR x86IOAPIC"LibIoApic relay: maskIrq: __kpin %d "
		"doesn't map to any known IO-APIC.\n",
		__kpin);
}

void x86IoApic::unmaskIrq(ubit16 __kpin)
{
	IoApic		*ioApic;

	ioApic = getIoApicFor(__kpin);
	if (ioApic != NULL)
	{
		ioApic->unmaskIrq(__kpin);
		return;
	};

	printf(ERROR x86IOAPIC"LibIoApic relay: unmaskIrq: __kpin %d "
		"doesn't map to any known IO-APIC.\n",
		__kpin);
}

#endif

