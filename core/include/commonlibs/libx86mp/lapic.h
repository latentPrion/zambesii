#ifndef _x86_LOCAL_APIC_H
	#define _x86_LOCAL_APIC_H

	#include <arch/paddr_t.h>
	#include <__kstdlib/__ktypes.h>

#define x86LAPIC		"LAPIC: "
// "LAPIC 00"
#define x86LAPIC_MAGIC		0x1A1D1C00

// Offsets to LAPIC registers.
#define x86LAPIC_REG_LAPICID		0x20
#define x86LAPIC_REG_LAPIC_VER		0x30
#define x86LAPIC_REG_TASKPRIO		0x80
#define x86LAPIC_REG_ARBIT_PRIO		0x90
#define x86LAPIC_REG_PROC_PRIO		0xA0
#define x86LAPIC_REG_EOI		0xB0
#define x86LAPIC_REG_REMOTE_READ	0xC0
#define x86LAPIC_REG_LOG_DEST		0xD0
#define x86LAPIC_REG_DEST_FMT		0xE0
#define x86LAPIC_REG_SPURIOUS_VECT	0xF0
#define x86LAPIC_REG_INSERV0		0x100
#define x86LAPIC_REG_INSERV1		0x110
#define x86LAPIC_REG_INSERV2		0x120
#define x86LAPIC_REG_INSERV3		0x130
#define x86LAPIC_REG_INSERV4		0x140
#define x86LAPIC_REG_INSERV5		0x150
#define x86LAPIC_REG_INSERV6		0x160
#define x86LAPIC_REG_INSERV7		0x170
#define x86LAPIC_REG_TRIGG_MODE0	0x180
#define x86LAPIC_REG_TRIGG_MODE1	0x190
#define x86LAPIC_REG_TRIGG_MODE2	0x1A0
#define x86LAPIC_REG_TRIGG_MODE3	0x1B0
#define x86LAPIC_REG_TRIGG_MODE4	0x1C0
#define x86LAPIC_REG_TRIGG_MODE5	0x1D0
#define x86LAPIC_REG_TRIGG_MODE6	0x1E0
#define x86LAPIC_REG_TRIGG_MODE7	0x1F0
#define x86LAPIC_REG_INT_REQ0		0x200
#define x86LAPIC_REG_INT_REQ1		0x210
#define x86LAPIC_REG_INT_REQ2		0x220
#define x86LAPIC_REG_INT_REQ3		0x230
#define x86LAPIC_REG_INT_REQ4		0x240
#define x86LAPIC_REG_INT_REQ5		0x250
#define x86LAPIC_REG_INT_REQ6		0x260
#define x86LAPIC_REG_INT_REQ7		0x270
#define x86LAPIC_REG_ERRSTAT		0x280
#define x86LAPIC_REG_LVT_CMCI		0x2F0
#define x86LAPIC_REG_INT_CMD0		0x300
#define x86LAPIC_REG_INT_CMD1		0x310
#define x86LAPIC_REG_LVT_TIMER		0x320
#define x86LAPIC_REG_LVT_THERM		0x330
#define x86LAPIC_REG_LVT_PERF		0x340
#define x86LAPIC_REG_LVT_LINT0		0x350
#define x86LAPIC_REG_LVT_LINT1		0x360
#define x86LAPIC_REG_LVT_ERR		0x370
#define x86LAPIC_REG_INIT_COUNT		0x380
#define x86LAPIC_REG_CURR_COUNT		0x390
#define x86LAPIC_REG_DIV_CFG		0x3E0

#define x86LAPIC_IPI_

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

	// IPI-related functions.
	void sendPhysicalIpi(ubit8 type, ubit8 vector, ubit8 dest);
	void sendClusterIpi(ubit8 type, ubit8 vector, ubit8 cluster, ubit8 cpu);
	void sendFlatLogicalIpi(ubit8 type, ubit8 vector, ubit8 mask);
}


#endif

