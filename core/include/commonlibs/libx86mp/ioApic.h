#ifndef _LIB_x86MP_IO_APIC_H
	#define _LIB_x86MP_IO_APIC_H

	#include <arch/paddr_t.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/hardwareIdList.h>

	#define x86IOAPIC			"IO-APIC: "

	#define x86IOAPIC_MAGIC			(0x10A1D1C0)

namespace x86IoApic
{
	class ioApicC
	{
	public:
		ioApicC(ubit8, paddr_t){};
		error_t initialize(void){return ERROR_SUCCESS;};
		~ioApicC(void);

	public:
		ubit8 getNIrqs(void);
		void getIrqState(ubit8 irq);
		error_t setIrqState(ubit8 irq);

		void maskIrq(ubit8 irq);
		void unmaskIrq(ubit8 irq);

	private:
		paddr_t		paddr;
		ubit8		version;
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

	sarch_t ioApicsAreMapped(void);
	error_t mapIoApics(void);
	ubit16 getNIoApics(void);

	ioApicC *getIoApic(ubit8 id);
}

#endif

