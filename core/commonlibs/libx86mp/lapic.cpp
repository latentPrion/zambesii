
#include <arch/paddr_t.h>
#include <arch/walkerPageRanger.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kclasses/debugPipe.h>
#include <commonlibs/libx86mp/lapic.h>
#include <commonlibs/libx86mp/mpTables.h>
#include <kernel/common/memoryTrib/memoryTrib.h>


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

	// Map the LAPIC per-contextual paddr to a the kernel's vaddrspace.
	v = (memoryTrib.__kmemoryStream.vaddrSpaceStream
		.*memoryTrib.__kmemoryStream.vaddrSpaceStream.getPages)(
		x86_LAPIC_NPAGES);

	if (v == __KNULL)
	{
		__kprintf(ERROR x86LAPIC"Failed to get pages to map LAPIC.\n");
		return ERROR_MEMORY_NOMEM_VIRTUAL;
	};

	nMapped = walkerPageRanger::mapInc(
		&memoryTrib.__kmemoryStream.vaddrSpaceStream.vaddrSpace,
		v, p, x86_LAPIC_NPAGES,
		PAGEATTRIB_PRESENT | PAGEATTRIB_WRITE | PAGEATTRIB_SUPERVISOR
		| PAGEATTRIB_CACHE_WRITE_THROUGH);

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

void x86Lapic::lintSetup(ubit8 lint, ubit8 type, ubit32 flags, ubit8 vector)
{
	ubit32		outval=0;

	outval |= lintSetup_lvtConvertType(type) << x86LAPIC_LVT_INTTYPE_SHIFT;
	outval |= lintSetup_lvtConvertPolarity(flags)
		<< x86LAPIC_LVT_POLARITY_SHIFT;

	switch (type)
	{
	case x86LAPIC_LINT_TYPE_INT:
		// Vector and trigger mode only apply for fixed interrupts.
		outval |= vector;
		outval |= lintSetup_lvtConvertTriggerMode(flags)
			<< x86LAPIC_LVT_TRIGGERMODE_SHIFT;

		break;

	case x86LAPIC_LINT_TYPE_EXTINT:
		// ExtINT is always level triggered.
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
	return *reinterpret_cast<ubit8 *>( (uarch_t)cache.v + offset );
}

ubit16 x86Lapic::read16(ubit32 offset)
{
	return *reinterpret_cast<ubit16 *>( (uarch_t)cache.v + offset );
}

ubit32 x86Lapic::read32(ubit32 offset)
{
	return *reinterpret_cast<ubit32 *>( (uarch_t)cache.v + offset );
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
	*reinterpret_cast<ubit32 *>( (uarch_t)cache.v + offset ) = val;
}

