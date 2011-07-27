#ifndef _x86_LOCAL_APIC_H
	#define _x86_LOCAL_APIC_H

	#include <arch/paddr_t.h>
	#include <__kstdlib/__ktypes.h>

#define x86LAPIC		"LAPIC: "
// "LAPIC 00"
#define x86LAPIC_MAGIC		0x1A1D1C00

struct x86LapicCacheS
{
	ubit32		magic;
	ubit8		*v;
	paddr_t		p;
};


namespace x86Lapic
{
	void initializeCache(void);
	void flushCache(void);

	sarch_t getPaddr(paddr_t *p);
	void setPaddr(paddr_t p);
	error_t mapLapicMem(void);

	ubit8 read8(ubit32 offset);
	ubit16 read16(ubit32 offset);
	ubit32 read32(ubit32 offset);

	void write8(ubit32 offset, ubit8 val);
	void write16(ubit32 offset, ubit16 val);
	void write32(ubit32 regOffset, ubit32 value);
}


#endif

