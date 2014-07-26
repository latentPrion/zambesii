#ifndef _x86_LOCAL_APIC_H
	#define _x86_LOCAL_APIC_H

#ifndef __ASM__
	#include <chipset/zkcm/device.h>
	#include <arch/paddr_t.h>
	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/smpTypes.h>
	#include <kernel/common/interruptTrib/__kexceptionFn.h>
	#include "mpTables.h"
#endif

#define x86LAPIC		"LAPIC: "
/* "LAPIC 00" */
#define x86LAPIC_MAGIC		0x1A1D1C00

/* Offsets to LAPIC registers. */
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

#define x86LAPIC_VECTOR_SPURIOUS	0xFF
/* x86 IPI vector. 0xFE is one int priority below the highest on LAPIC. */
#define x86LAPIC_VECTOR_IPI		0xFE
#define x86LAPIC_VECTOR_LVT_ERROR	0xFD

#define x86LAPIC_IPI_DELIVERY_STATUS_PENDING	(1<<12)

#define x86LAPIC_IPI_TYPE_SHIFT			8
#define x86LAPIC_IPI_TYPE_FIXED			0x0
#define x86LAPIC_IPI_TYPE_LOWPRIO		0x1
#define x86LAPIC_IPI_TYPE_SMI			0x2
#define x86LAPIC_IPI_TYPE_NMI			0x4
/* These two have the same value. Also when sending INIT, always use vector 0 */
#define x86LAPIC_IPI_TYPE_INIT			0x5
#define x86LAPIC_IPI_TYPE_ARBIT_RESET		0x5
#define x86LAPIC_IPI_TYPE_SIPI			0x6

#define x86LAPIC_IPI_DESTMODE_SHIFT		11
#define x86LAPIC_IPI_DESTMODE_PHYS		0x0
#define x86LAPIC_IPI_DESTMODE_LOG		0x1

#define x86LAPIC_IPI_LEVEL_SHIFT		14
#define x86LAPIC_IPI_LEVEL_ARBIT_RESET		0x0
#define x86LAPIC_IPI_LEVEL_OTHER		0x1

#define x86LAPIC_IPI_TRIGG_SHIFT		15
#define x86LAPIC_IPI_TRIGG_EDGE			0x0
#define x86LAPIC_IPI_TRIGG_LEVEL		0x1

#define x86LAPIC_IPI_SHORTDEST_SHIFT		18
#define x86LAPIC_IPI_SHORTDEST_NONE		0x0
#define x86LAPIC_IPI_SHORTDEST_SELF		0x1
#define x86LAPIC_IPI_SHORTDEST_BROAD_INC	0x2
#define x86LAPIC_IPI_SHORTDEST_BROAD_EXC	0x3

#define x86LAPIC_LINT_TYPE_NMI			0x0
#define x86LAPIC_LINT_TYPE_EXTINT		0x1
#define x86LAPIC_LINT_TYPE_INT			0x2
#define x86LAPIC_LINT_TYPE_SMI			0x3


#ifndef __ASM__
class x86LapicC
:
public zkcmDeviceC
{
public:
	x86LapicC(class cpuStreamC *parent);
	error_t initialize(void) { return ERROR_SUCCESS; }
	~x86LapicC(void) {}

	// zkcmTimerDeviceC *getTimer(void) { return &timer; }

public:
	// Used to set up LAPICs on each CPU when they are booted.
	error_t setupLapic(void);

	sarch_t cpuHasLapic(void);
	static error_t mapLapicMem(void);
	static sarch_t lapicMemIsMapped(void);

	static error_t detectPaddr(void);
	static void setPaddr(paddr_t p)
	{
		cache.p = p;
	}

	static sarch_t getPaddr(paddr_t *p)
	{
		*p = cache.p;
		return (cache.p != 0);
	}

	/* Lapic Global enable/disable (IA32_APIC_BASE[11]).
	 * On 3-wire LAPIC-bus chips, this is irreversible without chipset
	 * reboot. On system-bus chips this is reversible as long as no
	 * interrupts are triggered mid-operation.
	 **/
	static void globalEnable(void);
	static void globalDisable(void);
	static sarch_t isGloballyEnabled(void);

	// Per-LAPIC soft enable/disable.
	void softEnable(void);
	void softDisable(void);
	sarch_t isSoftEnabled(void);

	void sendEoi(void);

	inline ubit8 getVersion(void)
		{ return read32(x86LAPIC_REG_LAPIC_VER) & 0xFF; }

	inline sarch_t isOldNonIntegratedLapic(void)
	{
#ifdef CONFIG_ARCH_x86_32
		return !(getVersion() & 0xF0u);
#else
		return 0;
#endif
	}


	ubit32 read32(ubit32 offset);
	void write32(ubit32 regOffset, ubit32 value);

	// IPI-related functions.
	struct sIpi
	{
		error_t setupIpis(class cpuStreamC *parent);

		error_t sendPhysicalIpi(
			ubit8 type, ubit8 vector, ubit8 shortDest, cpu_t dest);

		void sendClusterIpi(
			ubit8 type, ubit8 vector, ubit8 cluster, ubit8 cpu);

		void sendFlatLogicalIpi(ubit8 type, ubit8 vector, ubit8 mask);

	private:
		void installHandler(void);
		static __kexceptionFn	exceptionHandler;
		static sarch_t		handlerIsInstalled;
	} ipi;

	struct sLint
	{
		error_t setupLints(class cpuStreamC *parent);

		void sLintetup(
			class cpuStreamC *parent,
			ubit8 lint, ubit8 intType, ubit32 flags, ubit8 vector);

		void lintEnable(class cpuStreamC *parent, ubit8 lint);
		void lintDisable(class cpuStreamC *parent, ubit8 lint);

		/* LINT pin related functions.
		 *
		 * The "flags" arg expects the flags to be in the same format
		 * as the MP specification's "flags" field for the Local
		 * Interrupt Source entries.
		 **/
		static ubit8 lintConvertMpCfgType(ubit8);
		static ubit32 lintConvertMpCfgFlags(ubit32);
		static ubit32 lintConvertAcpiFlags(ubit32);

	private:
		void sRsdtetupLints(class cpuStreamC *parent);
		void sXsdtetupLints(class cpuStreamC *parent);
	} lint;

	// This must always be 0xHF, where H is any hex digit, and F is fixed.
	struct sSpurious
	{
		error_t setupSpuriousVector(class cpuStreamC *parent);

	private:
		void installHandler(void);
		static __kexceptionFn	exceptionHandler;
		static sarch_t		handlerIsInstalled;
	} spurious;

	struct sError
	{
		error_t setupLvtError(class cpuStreamC *parent);

	private:
		void installHandler(void);
		static __kexceptionFn	exceptionHandler;
		static sarch_t		handlerIsInstalled;
	} error;	

	struct sCache
	{
		sCache(void)
		:
		magic(x86LAPIC_MAGIC),
		v(NULL)
		{
			memset(&p, 0, sizeof(p));
		}

		ubit32		magic;
		ubit8		*v;
		paddr_t		p;
	};

private:
	static sCache		cache;
	class cpuStreamC	*parent;
	// lapicTimerC		timer;
};
#endif

#endif

