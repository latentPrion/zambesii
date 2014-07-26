
#include <commonlibs/libx86mp/libx86mp.h>
#include <commonlibs/libacpi/libacpi.h>
#include <kernel/common/cpuTrib/cpuStream.h>


static void lintParseRMadtForEntries(acpi_rMadtS *rmadt, cpuStream *parent)
{
	void			*handle;
	acpi_rMadtLapicNmiS	*nmiEntry;

	handle = NULL;
	nmiEntry = acpiRMadt::getNextLapicNmiEntry(rmadt, &handle);
	while (nmiEntry != NULL)
	{
		// If this entry pertains to this CPU:
		if (nmiEntry->acpiLapicId == parent->cpuAcpiId
			|| nmiEntry->acpiLapicId == ACPI_MADT_NMI_LAPICID_ALL)
		{
			printf(NOTICE CPUSTREAM"%d: ACPI NMI: lint %d.\n",
				parent->cpuId, nmiEntry->lapicLint);

			parent->lapic.lint.sLintetup(
				parent,
				nmiEntry->lapicLint,
				x86LAPIC_LINT_TYPE_NMI,
				x86LapicC::sLint::lintConvertAcpiFlags(
					nmiEntry->flags),
				0);

			parent->lapic.lint.lintEnable(
				parent, nmiEntry->lapicLint);
		};

		nmiEntry = acpiRMadt::getNextLapicNmiEntry(rmadt, &handle);
	};
}

#include <kernel/common/cpuTrib/cpuTrib.h>
void x86LapicC::sLint::sRsdtetupLints(cpuStream *parent)
{
	acpi_sRsdt		*rsdt;
	acpi_rMadtS		*rmadt;
	void			*context, *handle;

	rsdt = acpi::getRsdt();
	context = handle = NULL;
	rmadt = acpiRsdt::getNextMadt(rsdt, &context, &handle);

int	i=0;
	while (rmadt != NULL)
	{
i++;
if (!FLAG_TEST(cpuTrib.getCurrentCpuStream()->flags, CPUSTREAM_FLAGS_BSP))
{
	printf(NOTICE"CPU %d: madt at 0x%p.\n", cpuTrib.getCurrentCpuStream()->cpuId, rmadt);
};
		lintParseRMadtForEntries(rmadt, parent);

		acpiRsdt::destroySdt((acpi_sdtS *)rmadt);
		rmadt = acpiRsdt::getNextMadt(
			rsdt, &context, &handle);
	};
if (!FLAG_TEST(cpuTrib.getCurrentCpuStream()->flags, CPUSTREAM_FLAGS_BSP))
{
	asm volatile("cli\n\thlt\n\t");
};
}

#include <debug.h>
error_t x86LapicC::sLint::setupLints(cpuStream *parent)
{
	uarch_t				pos=0;
	void				*handle;
	x86_mpCfgLocalIrqSourceS	*lintEntry;

	/**	EXPLANATION:
	 * We actually use both the MP Tables and the APIC MADT to set up the
	 * LAPIC Lints. This is because the MP Tables have information about
	 * both extInt and NMI, while ACPI only has information about NMI. But
	 * we want both sets of information.
	 **/
	x86Mp::initializeCache();
	if (x86Mp::findMpFp() == NULL)
	{
		goto useAcpi;
	};

	if (x86Mp::mapMpConfigTable() == NULL)
	{
		printf(WARNING x86LAPIC"Failed to map MP config table.\n");
		goto useAcpi;
	};

	lintEntry = x86Mp::getNextLocalIrqSourceEntry(&pos, &handle);
	while (lintEntry != NULL)
	{
		// If the entry pertains to this CPU:
		if (lintEntry->destLapicId == parent->cpuId
			|| lintEntry->destLapicId == x86_MPCFG_LIRQSRC_DEST_ALL)
		{
			printf(NOTICE CPUSTREAM"%d: MPCFG: lint %d, type %d."
				"\n", parent->cpuId,
				lintEntry->destLapicLint,
				lintEntry->intType);

			parent->lapic.lint.sLintetup(
				parent,
				lintEntry->destLapicLint,
				x86LapicC::sLint::lintConvertMpCfgType(
					lintEntry->intType),
				x86LapicC::sLint::lintConvertMpCfgFlags(
					lintEntry->flags),
				0);

			parent->lapic.lint.lintEnable(
				parent, lintEntry->destLapicLint);
		};

		lintEntry = x86Mp::getNextLocalIrqSourceEntry(
			&pos, &handle);
	};

useAcpi:
	acpi::initializeCache();
	if (acpi::findRsdp() == ERROR_SUCCESS)
	{
#if !defined(__32_BIT__) || defined(CONFIG_ARCH_x86_32_PAE)
		if (acpi::testForXsdt())
		{
		};
#endif

		if (acpi::testForRsdt())
		{
			if (acpi::mapRsdt() != ERROR_SUCCESS)
			{
				printf(ERROR CPUSTREAM"%d: Unable to map "
					"RSDT.\n",
					parent->cpuId);

				return ERROR_SUCCESS;
			};

			sRsdtetupLints(parent);
			return ERROR_SUCCESS;
		};
	};

	return ERROR_SUCCESS;
}

ubit8 x86LapicC::sLint::lintConvertMpCfgType(ubit8 mpTypeField)
{
	switch (mpTypeField)
	{
	case x86_MPCFG_LIRQSRC_INTTYPE_INT:
		return x86LAPIC_LINT_TYPE_INT;

	case x86_MPCFG_LIRQSRC_INTTYPE_NMI:
		return x86LAPIC_LINT_TYPE_NMI;

	case x86_MPCFG_LIRQSRC_INTTYPE_SMI:
		return x86LAPIC_LINT_TYPE_SMI;

	case x86_MPCFG_LIRQSRC_INTTYPE_EXTINT:
		return x86LAPIC_LINT_TYPE_EXTINT;

	default: return x86LAPIC_LINT_TYPE_INT;
	};
}

// Both ACPI and MP tables use the same flag bit positions, so we will too.
ubit32 x86LapicC::sLint::lintConvertMpCfgFlags(ubit32 mpFlagField)
{
	return mpFlagField;
}

ubit32 x86LapicC::sLint::lintConvertAcpiFlags(ubit32 acpiFlagField)
{
	return acpiFlagField;
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

static inline ubit8 sLintetup_lvtConvertType(ubit8 type)
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

static inline ubit8 sLintetup_lvtConvertPolarity(ubit32 flags)
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

static inline ubit8 sLintetup_lvtConvertTriggerMode(ubit32 flags)
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

void x86LapicC::sLint::lintEnable(cpuStream *parent, ubit8 lint)
{
	ubit32		outval;

	if (lint > 1) { return; };

	outval = parent->lapic.read32(
		(lint == 0) ? x86LAPIC_REG_LVT_LINT0 : x86LAPIC_REG_LVT_LINT1);

	// Just unset the mask bit and rewrite what was already there.
	FLAG_UNSET(outval, x86LAPIC_LVT_FLAGS_DISABLED);

	parent->lapic.write32(
		(lint == 0) ? x86LAPIC_REG_LVT_LINT0 : x86LAPIC_REG_LVT_LINT1,
		outval);
}

void x86LapicC::sLint::lintDisable(cpuStream *parent, ubit8 lint)
{
	ubit32		outval;

	if (lint > 1) { return; };
	outval = parent->lapic.read32(
		(lint == 0) ? x86LAPIC_REG_LVT_LINT0 : x86LAPIC_REG_LVT_LINT1);

	// Just set the mask bit and rewrite what was already there.
	FLAG_SET(outval, x86LAPIC_LVT_FLAGS_DISABLED);

	parent->lapic.write32(
		(lint == 0) ? x86LAPIC_REG_LVT_LINT0 : x86LAPIC_REG_LVT_LINT1,
		outval);
}

void x86LapicC::sLint::sLintetup(
	cpuStream *parent,
	ubit8 lint, ubit8 deliveryMode, ubit32 flags, ubit8 vector
	)
{
	ubit32		outval=0;

	if (lint > 1) { return; };

	outval |= sLintetup_lvtConvertType(deliveryMode)
		<< x86LAPIC_LVT_INTTYPE_SHIFT;

	outval |= sLintetup_lvtConvertPolarity(flags)
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
		outval |= sLintetup_lvtConvertTriggerMode(flags)
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
	parent->lapic.write32(
		(lint == 0) ? x86LAPIC_REG_LVT_LINT0 : x86LAPIC_REG_LVT_LINT1,
		outval);
}

