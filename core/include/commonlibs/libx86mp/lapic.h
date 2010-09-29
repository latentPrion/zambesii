#ifndef _x86_LOCAL_APIC_H
	#define _x86_LOCAL_APIC_H

	#include <__kstdlib/__ktypes.h>

#define x86LAPIC		"LAPIC: "

struct x86LapicCacheS
{
	ubit32		*v;
};


namespace x86Lapic
{
	error_t mapLapicMem(void);

	inline ubit8 read8(ubit32 offset);
	inline ubit16 read16(ubit32 offset);
	inline ubit32 read32(ubit32 offset);

	inline void write8(ubit32 offset, ubit8 val);
	inline void write16(ubit32 offset, ubit16 val);
	inline void write32(ubit32 regOffset, ubit32 value);
}

#endif

