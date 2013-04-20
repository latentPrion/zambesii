
#include <arch/paddr_t.h>
#include <arch/walkerPageRanger.h>
#include <arch/x8632/cpuid.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kclasses/debugPipe.h>
#include <commonlibs/libx86mp/lapic.h>
#include <commonlibs/libx86mp/mpTables.h>
#include <commonlibs/libacpi/libacpi.h>
#include <kernel/common/processTrib/processTrib.h>


#define x86_LAPIC_NPAGES		4

static struct x86Lapic::cacheS		cache;

void x86Lapic::initializeCache(void)
{
	if (cache.magic != x86LAPIC_MAGIC)
	{
		x86Lapic::flushCache();
		cache.magic = x86LAPIC_MAGIC;
	};
}

void x86Lapic::flushCache(void)
{
	memset8(&cache, 0, sizeof(cache));
}

void x86Lapic::setPaddr(paddr_t p)
{
	cache.p = p;
}

sarch_t x86Lapic::getPaddr(paddr_t *p)
{
	*p = cache.p;
	return (cache.p != 0);
}

error_t x86Lapic::detectPaddr(void)
{
	x86_mpCfgS		*cfgTable;
	acpi_rsdtS		*rsdt;
	acpi_rMadtS		*madt;
	void			*handle, *context;
	paddr_t			tmp;

	if (getPaddr(&tmp)) { return ERROR_SUCCESS; };

	acpi::initializeCache();
	if (acpi::findRsdp() != ERROR_SUCCESS) { goto tryMpTables; };
#if !defined(__32_BIT__) || defined(CONFIG_ARCH_x86_32_PAE)
	// If not 32 bit, try XSDT.
	if (acpi::testForXsdt())
	{
		// Map XSDT, etc.
	}
	else // Continues into an else-if for RSDT test.
#endif
	// 32 bit uses RSDT only.
	if (acpi::testForRsdt())
	{
		if (acpi::mapRsdt() != ERROR_SUCCESS) { goto tryMpTables; };
		
		rsdt = acpi::getRsdt();
		context = handle = __KNULL;
		madt = acpiRsdt::getNextMadt(rsdt, &context, &handle);

		if (madt == __KNULL) { goto tryMpTables; };
		tmp = (paddr_t)madt->lapicPaddr;

		acpiRsdt::destroyContext(&context);
		acpiRsdt::destroySdt(reinterpret_cast<acpi_sdtS *>( madt ));

		// Have the LAPIC paddr. Move on.
		goto initLibLapic;
	}
	else
	{
		__kprintf(WARNING x86LAPIC"detectPaddr(): RSDP found, but no "
			"RSDT or XSDT.\n");
	};

tryMpTables:
	x86Mp::initializeCache();
	if (!x86Mp::mpConfigTableIsMapped())
	{
		x86Mp::findMpFp();
		if (!x86Mp::mpFpFound()) {
			goto useDefaultPaddr;
		};

		if (x86Mp::mapMpConfigTable() == __KNULL) {
			goto useDefaultPaddr;
		};
	};

	cfgTable = x86Mp::getMpCfg();
	tmp = cfgTable->lapicPaddr;
	goto initLibLapic;

useDefaultPaddr:
	__kprintf(WARNING x86LAPIC"detectPaddr: Using default paddr.\n");
	tmp = 0xFEE00000;

initLibLapic:
	__kprintf(NOTICE x86LAPIC"detectPaddr: LAPIC paddr: 0x%P.\n",
		tmp);

	x86Lapic::setPaddr(tmp);
	return ERROR_SUCCESS;
}

sarch_t x86Lapic::cpuHasLapic(void)
{
	uarch_t			eax, ebx, ecx, edx;

	execCpuid(1, &eax, &ebx, &ecx, &edx);
	if (!(edx & (1 << 9)))
	{
		__kprintf(ERROR x86LAPIC"cpuHasLapic: CPUID[1].EDX[9] "
			"LAPIC existence check failed. EDX: %x.\n",
			edx);

		return 0;
	};

	return 1;
}

sarch_t x86Lapic::lapicMemIsMapped(void)
{
	if (cache.magic == x86LAPIC_MAGIC && cache.v != __KNULL) {
		return 1;
	};
	return 0;
}

error_t x86Lapic::mapLapicMem(void)
{
	paddr_t		p = cache.p;
	void		*v;
	status_t	nMapped;

	if (cache.v != __KNULL) { return ERROR_SUCCESS; };

	// Map the LAPIC per-contextual paddr to a the kernel's vaddrspace.
	v = processTrib.__kprocess.memoryStream.vaddrSpaceStream.getPages(
		x86_LAPIC_NPAGES);

	if (v == __KNULL)
	{
		__kprintf(ERROR x86LAPIC"Failed to get pages to map LAPIC.\n");
		return ERROR_MEMORY_NOMEM_VIRTUAL;
	};

	// Fact worth noting: Linux maps the APICs as uncacheable.
	nMapped = walkerPageRanger::mapInc(
		&processTrib.__kprocess.memoryStream.vaddrSpaceStream.vaddrSpace,
		v, p, x86_LAPIC_NPAGES,
		PAGEATTRIB_PRESENT | PAGEATTRIB_WRITE | PAGEATTRIB_SUPERVISOR
		| PAGEATTRIB_CACHE_DISABLED);

	if (nMapped < x86_LAPIC_NPAGES)
	{
		__kprintf(ERROR x86LAPIC"Failed to map LAPIC at p 0x%X.\n", p);
		return ERROR_MEMORY_VIRTUAL_PAGEMAP;
	};

	v = WPRANGER_ADJUST_VADDR(v, p, void *);
	cache.v = static_cast<ubit8 *>( v );
	return ERROR_SUCCESS;
}

#define x86LAPIC_FLAG_SOFT_ENABLE		(1<<8)

void x86Lapic::softEnable(void)
{
	ubit32		outval;

	// Set the enabled flag in the spurious int vector reg.
	outval = x86Lapic::read32(x86LAPIC_REG_SPURIOUS_VECT);
	__KFLAG_SET(outval, x86LAPIC_FLAG_SOFT_ENABLE);
	x86Lapic::write32(x86LAPIC_REG_SPURIOUS_VECT, outval);
}

void x86Lapic::softDisable(void)
{
	ubit32		outval;

	outval = x86Lapic::read32(x86LAPIC_REG_SPURIOUS_VECT);
	__KFLAG_UNSET(outval, x86LAPIC_FLAG_SOFT_ENABLE);
	x86Lapic::write32(x86LAPIC_REG_SPURIOUS_VECT, outval);
}

#define x86LAPIC_LVT_FLAGS_DISABLED		(1<<16)

#define x86LAPIC_LVT_VECTOR_SHIFT		0

#define x86LAPIC_LVT_INTTYPE_SHIFT		8
#define x86LAPIC_LVT_INTTYPE_FIXED		0x0
#define x86LAPIC_LVT_INTTYPE_SMI		0x2
#define x86LAPIC_LVT_INTTYPE_NMI		0x4
#define x86LAPIC_LVT_INTTYPE_EXTINT		0x7
#define x86LAPIC_LVT_INTTYPE_INIT		0x5

#define x86LAPIC_LVT_POLARITY_SHIFT		13
#define x86LAPIC_LVT_POLARITY_HIGH		0x0
#define x86LAPIC_LVT_POLARITY_LOW		0x1

#define x86LAPIC_LVT_TRIGGERMODE_SHIFT		15
#define x86LAPIC_LVT_TRIGGERMODE_EDGE		0x0
#define x86LAPIC_LVT_TRIGGERMODE_LEVEL		0x1

static inline ubit8 lintSetup_lvtConvertType(ubit8 type)
{
	switch (type)
	{
	case x86LAPIC_LINT_TYPE_SMI: return x86LAPIC_LVT_INTTYPE_SMI;
	case x86LAPIC_LINT_TYPE_NMI: return x86LAPIC_LVT_INTTYPE_NMI;
	case x86LAPIC_LINT_TYPE_EXTINT: return x86LAPIC_LVT_INTTYPE_EXTINT;
	case x86LAPIC_LINT_TYPE_INT: return x86LAPIC_LVT_INTTYPE_FIXED;
	default: return x86LAPIC_LVT_INTTYPE_FIXED;
	}
}

static inline ubit8 lintSetup_lvtConvertPolarity(ubit32 flags)
{
	// Polarity is in the first 2 bits.
	switch (flags & 0x3)
	{
	// Active high:
	case 0x1: return x86LAPIC_LVT_POLARITY_HIGH;
	// Active low:
	case 0x3: return x86LAPIC_LVT_POLARITY_LOW;
	/* For the "conforms to polarity of bus option, I set it to active low
	 * because it's highly likely that a LINT pin is going to get IRQs from
	 * the ISA/EISA part of the chipset. ISA is level triggered, active low.
	 **/
	default: return x86LAPIC_LVT_POLARITY_LOW;
	}
}

static inline ubit8 lintSetup_lvtConvertTriggerMode(ubit32 flags)
{
	switch ((flags >> 2) & 0x3)
	{
	// Edge triggered.
	case 0x1: return x86LAPIC_LVT_TRIGGERMODE_EDGE;
	// Level triggered.
	case 0x3: return x86LAPIC_LVT_TRIGGERMODE_LEVEL;
	/* Same as with the polarity: more likely that something ISA related is
	 * connected to the LAPIC LINT pins than something PCI related or
	 * otherwise.
	 **/
	default: return x86LAPIC_LVT_TRIGGERMODE_LEVEL;
	}
}

void x86Lapic::lintEnable(ubit8 lint)
{
	ubit32		outval;

	outval = x86Lapic::read32(
		(lint == 0) ? x86LAPIC_REG_LVT_LINT0 : x86LAPIC_REG_LVT_LINT1);

	// Just unset the mask bit and rewrite what was already there.
	__KFLAG_UNSET(outval, x86LAPIC_LVT_FLAGS_DISABLED);

	x86Lapic::write32(
		(lint == 0) ? x86LAPIC_REG_LVT_LINT0 : x86LAPIC_REG_LVT_LINT1,
		outval);
}

void x86Lapic::lintDisable(ubit8 lint)
{
	ubit32		outval;

	outval = x86Lapic::read32(
		(lint == 0) ? x86LAPIC_REG_LVT_LINT0 : x86LAPIC_REG_LVT_LINT1);

	// Just set the mask bit and rewrite what was already there.
	__KFLAG_SET(outval, x86LAPIC_LVT_FLAGS_DISABLED);

	x86Lapic::write32(
		(lint == 0) ? x86LAPIC_REG_LVT_LINT0 : x86LAPIC_REG_LVT_LINT1,
		outval);
}

void x86Lapic::lintSetup(
	ubit8 lint, ubit8 deliveryMode, ubit32 flags, ubit8 vector
	)
{
	ubit32		outval=0;

	outval |= lintSetup_lvtConvertType(deliveryMode)
		<< x86LAPIC_LVT_INTTYPE_SHIFT;

	outval |= lintSetup_lvtConvertPolarity(flags)
		<< x86LAPIC_LVT_POLARITY_SHIFT;

	/* "Selects the trigger mode for the local LINT0 and LINT1 pins:
	 * (0) edge sensitive and (1) level sensitive. This flag is only
	 * used when the delivery mode is Fixed. When the delivery mode
	 * is NMI, SMI, or INIT, the trigger mode is always edge
	 * sensitive. When the delivery mode is ExtINT, the trigger mode
	 * is always level sensitive. The timer and error interrupts are
	 * always treated as edge sensitive."
	 *	-Intel Manuals Vol 3a, 10.6.1.
	 **/
	switch (deliveryMode)
	{
	case x86LAPIC_LINT_TYPE_INT:
		// Vector and trigger mode only apply for fixed interrupts.
		outval |= vector;
		outval |= lintSetup_lvtConvertTriggerMode(flags)
			<< x86LAPIC_LVT_TRIGGERMODE_SHIFT;

		break;

	case x86LAPIC_LINT_TYPE_EXTINT:
		// Trigger mode is always level for extInt.
		outval |= x86LAPIC_LVT_TRIGGERMODE_LEVEL
			<< x86LAPIC_LVT_TRIGGERMODE_SHIFT;

		break;

	// Everything else is edge triggered.
	default: outval |= x86LAPIC_LVT_TRIGGERMODE_EDGE
		<< x86LAPIC_LVT_TRIGGERMODE_SHIFT;

		break;
	};

	// Finally, write the value out.
	write32(
		(lint == 0) ? x86LAPIC_REG_LVT_LINT0 : x86LAPIC_REG_LVT_LINT1,
		outval);
}

void x86Lapic::sendEoi(void)
{
	write32(x86LAPIC_REG_EOI, 0);
}

#define x86LAPIC_LVT_ERR_FLAGS_DISABLE	(1<<16)

void x86Lapic::setupLvtError(ubit8 vector)
{
	x86Lapic::write32(x86LAPIC_REG_LVT_ERR, 0 | vector);
}

#define x86LAPIC_SPURIOUS_VECTOR_FLAGS_DISABLE		(1<<8)

void x86Lapic::setupSpuriousVector(ubit8 vector)
{
	ubit32		outval;

	outval = x86Lapic::read32(x86LAPIC_REG_SPURIOUS_VECT);
	x86Lapic::write32(x86LAPIC_REG_SPURIOUS_VECT, outval | vector);
}

ubit8 x86Lapic::read8(ubit32 offset)
{
	return *reinterpret_cast<volatile ubit8*>( (uarch_t)cache.v + offset );
}

ubit16 x86Lapic::read16(ubit32 offset)
{
	return *reinterpret_cast<volatile ubit16*>( (uarch_t)cache.v + offset );
}

ubit32 x86Lapic::read32(ubit32 offset)
{
	return *reinterpret_cast<volatile ubit32*>( (uarch_t)cache.v + offset );
}


void x86Lapic::write8(ubit32 offset, ubit8 val)
{
	*reinterpret_cast<ubit8 *>( (uarch_t)cache.v + offset ) = val;
}

void x86Lapic::write16(ubit32 offset, ubit16 val)
{
	*reinterpret_cast<ubit16 *>( (uarch_t)cache.v + offset ) = val;
}

void x86Lapic::write32(ubit32 offset, ubit32 val)
{
	// Linux uses XCHG to write to the LAPIC for certain hardware.
	*reinterpret_cast<ubit32 *>( (uarch_t)cache.v + offset ) = val;
}

