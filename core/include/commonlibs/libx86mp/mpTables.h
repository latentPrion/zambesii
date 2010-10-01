#ifndef _x86_32_MULTIPROCESSOR_SPEC_H
	#define _x86_32_MULTIPROCESSOR_SPEC_H

	#include <__kstdlib/__ktypes.h>

/**	EXPLANATION:
 * Header containing all header-specific things needed to find, parse, and
 * otherwise manipulate the data structures related directly to, and including
 * the Intel MP Floating Pointer structure and the MP Configuration Table.
 **/

#define x86MP				"x86 MP Config: "

#define x86_MPFP_SIG			"_MP_"
#define x86_MPFP_SIGLEN			4

#define x86_MPFP_FEAT1_CFGPRESENT	0

#define x86_MPFP_FEAT2_FLAG_PICMODE	(1<<7)

struct x86_mpFpS
{
	char		sig[4];
	ubit32		cfgTablePaddr;
	// Length in paragraphs. Includes all bytes starting from offset 0.
	ubit8		length;
	ubit8		specRev;
	ubit8		checksum;
	ubit8		features[5];
} __attribute__((packed));


#define x86_MPCFG_SIG			"PCMP"
#define x86_MPCFG_SIGLEN		4

struct x86_mpCfgS
{
	char		sig[4];
	// Include all bytes from offset 0 of this struct.
	ubit16		length;
	ubit8		specRev;
	ubit8		checksum;
	char		oemIdString[8];
	char		oemProductIdString[12];
	ubit32		oemTablePaddr;
	ubit16		oemTableLength;
	// Num entries in THIS table. Not OEM table.
	ubit16		nEntries;
	ubit32		lapicPaddr;
	ubit16		extTableNEntries;
	ubit8		extTableChecksum;
	ubit8		rsvd;
} __attribute__((packed));

// Usage: Pass the address of the 1st byte of proposed next entry.
#define x86_MPCFG_GET_TYPE(addr)	(*(ubit8 *)addr)

#define x86_MPCFG_TYPE_CPU		0x0
#define x86_MPCFG_TYPE_BUS		0x1
#define x86_MPCFG_TYPE_IOAPIC		0x2
#define x86_MPCFG_TYPE_IRQSOURCE	0x3
#define x86_MPCFG_TYPE_LOCALIRQSOURCE	0x4
// From: linux: arch/x86/include/asm/mpspec_def.h. Used by IBM for NUMA-Q.
#define x86_MPCFG_TYPE_NUMAQ		192


// CPU Entry specific #defines.
#define x86_MPCFG_CPU_FLAGS_ENABLED	(1<<0)
#define x86_MPCFG_CPU_FLAGS_BSP		(1<<1)

#define x86_MPCFG_CPU_STEPPING_MASK	(0xF)
#define x86_MPCFG_CPU_STEPPING_SHIFT	0
#define x86_MPCFG_CPU_MODEL_MASK	(0xF)
#define x86_MPCFG_CPU_MODEL_SHIFT	4
#define x86_MPCFG_CPU_FAMILY_MASK	(0xF)
#define x86_MPCFG_CPU_FAMILY_SHIFT	8

struct x86_mpCfgCpuS
{
	ubit8		type;
	ubit8		lapicId;
	ubit8		lapicVersion;
	ubit8		flags;
	ubit32		cpuModel;
	ubit32		featureFlags;
	ubit32		rsvd1;
	ubit32		rsvd2;
} __attribute__((packed));


#define x86_MPCFG_BUS_CBUS	"CBUS"
#define x86_MPCFG_BUS_CBUSII	"CBUSII"
#define x86_MPCFG_BUS_EISA	"EISA"
#define x86_MPCFG_BUS_FUTURE	"FUTURE"
#define x86_MPCFG_BUS_INTERN	"INTERN"
#define x86_MPCFG_BUS_ISA	"ISA"
#define x86_MPCFG_BUS_MBI	"MBI"
#define x86_MPCFG_BUS_MBII	"MBII"
#define x86_MPCFG_BUS_MCA	"MCA"
#define x86_MPCFG_BUS_MPI	"MPI"
#define x86_MPCFG_BUS_MPSA	"MPSA"
#define x86_MPCFG_BUS_NUBUS	"NUBUS"
#define x86_MPCFG_BUS_PCI	"PCI"
#define x86_MPCFG_BUS_PCMCIA	"PCMCIA"
#define x86_MPCFG_BUS_TC	"TC"
#define x86_MPCFG_BUS_VL	"VL"
#define x86_MPCFG_BUS_VME	"VME"
#define x86_MPCFG_BUS_XPRESS	"XPRESS"

struct x86_mpCfgBusS
{
	ubit8		type;
	ubit8		busId;
	char		busString[6];
} __attribute__((packed));


#define x86_MPCFG_IOAPIC_FLAGS_ENABLED	(1<<0)

struct x86_mpCfgIoApicS
{
	ubit8		type;
	ubit8		ioApicId;
	ubit8		ioApicVersion;
	ubit8		flags;
	ubit32		ioApicPaddr;
} __attribute__((packed));


#define x86_MPCFG_IRQSRC_FLAGS_POLARITY_MASK		0x3
#define x86_MPCFG_IRQSRC_FLAGS_POLARITY_SHIFT		0

#define x86_MPCFG_IRQSRC_POLARITY_CONFORMS		0x0
#define x86_MPCFG_IRQSRC_POLARITY_ACTIVEHIGH		0x1
#define x86_MPCFG_IRQSRC_POLARITY_ACTIVELOW		0x3

#define x86_MPCFG_IRQSRC_FLAGS_SENSITIVITY_MASK		0x3
#define x86_MPCFG_IRQSRC_FLAGS_SENSITIVITY_SHIFT	2

#define x86_MPCFG_IRQSRC_SENSITIVITY_CONFORMS		0x0
#define x86_MPCFG_IRQSRC_SENSITIVITY_EDGE		0x1
#define x86_MPCFG_IRQSRC_SENSITIVITY_LEVEL		0x3

#define x86_MPCFG_IRQSRC_INTTYPE_INT			0x0
#define x86_MPCFG_IRQSRC_INTTYPE_NMI			0x1
#define x86_MPCFG_IRQSRC_INTTYPE_SMI			0x2
#define x86_MPCFG_IRQSRC_INTTYPE_EXTINT			0x3

struct x86_mpCfgIrqSourceS
{
	ubit8		type;
	ubit8		intType;
	ubit16		flags;
	ubit8		sourceBusId;
	ubit8		sourceBusIrq;
	ubit8		destIoApicId;
	ubit8		destIoApicVector;
} __attribute__((packed));


#define x86_MPCFG_LIRQSRC_FLAGS_POLARITY_MASK		0x3
#define x86_MPCFG_LIRQSRC_FLAGS_POLARITY_SHIFT		0

#define x86_MPCFG_LIRQSRC_POLARITY_CONFORMS		0x0
#define x86_MPCFG_LIRQSRC_POLARITY_ACTIVEHIGH		0x1
#define x86_MPCFG_LIRQSRC_POLARITY_ACTIVELOW		0x3

#define x86_MPCFG_LIRQSRC_FLAGS_SENSITIVITY_MASK	0x3
#define x86_MPCFG_LIRQSRC_FLAGS_SENSITIVITY_SHIFT	2

#define x86_MPCFG_LIRQSRC_SENSITIVITY_CONFORMS		0x0
#define x86_MPCFG_LIRQSRC_SENSITIVITY_EDGE		0x1
#define x86_MPCFG_LIRQSRC_SENSITIVITY_LEVEL		0x3

#define x86_MPCFG_LIRQSRC_INTTYPE_INT			0x0
#define x86_MPCFG_LIRQSRC_INTTYPE_NMI			0x1
#define x86_MPCFG_LIRQSRC_INTTYPE_SMI			0x2
#define x86_MPCFG_LIRQSRC_INTTYPE_EXTINT		0x3

struct x86_mpCfgLocalIrqSourceS
{
	ubit8		type;
	ubit8		intType;
	ubit16		flags;
	ubit8		sourceBusId;
	ubit8		sourceBusIrq;
	ubit8		destLapicId;
	ubit8		destLapicVector;
} __attribute__((packed));


#define x86_MPCACHE_FLAGS_WAS_PICMODE		(1<<0)

struct x86_mpCacheS
{
	struct x86_mpFpS	*fp;
	struct x86_mpCfgS	*cfg;
	uarch_t			nCfgEntries;
	// 0 = no, 1-7 = yes, -1 = no MP.
	sarch_t			defaultConfig;
	ubit32			lapicPaddr;
	uarch_t			flags;
	char			oemId[12];
	char			oemProductId[16];
	ubit8			isInitialized;
};

#ifdef __cplusplus

namespace x86Mp
{
	// Sets cache to safe init state.
	void initializeCache(void);
	void flushCache(void);

	x86_mpFpS *findMpFp(void);
	sarch_t mpTablesFound(void);
	x86_mpCfgS *mapMpConfigTable(void);
	// Returns: neg if uninitialized. 0 if cfg table present. >0 if default.
	status_t getChipsetDefaultConfig(void);

	// Iterates through all CPU entries in the config.
	x86_mpCfgCpuS *getNextCpuEntry(uarch_t *pos, void **const handle);
	// Iterates through all I/O APIC entries in the config.
	x86_mpCfgIoApicS *getNextIoApicEntry(uarch_t *pos, void **const handle);

	// Following return stuff from the cache mostly.
	ubit32 getLapicPaddr(void);
	x86_mpFpS *getMpFp(void);
	x86_mpCfgS *getMpCfg(void);
}

#endif

#endif

