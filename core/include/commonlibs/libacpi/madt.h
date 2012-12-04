#ifndef _ACPI_MADT_H
	#define _ACPI_MADT_H

	#include <__kstdlib/__ktypes.h>
	#include "baseTables.h"
	#include "mainTables.h"


#define ACPI_MADT_GET_TYPE(_addr)			(*(ubit8 *)_addr)
#define ACPI_MADT_TYPE_LAPIC				0
#define ACPI_MADT_TYPE_IOAPIC				1
#define ACPI_MADT_TYPE_IRQSOURCE_OVERRIDE		2
#define ACPI_MADT_TYPE_NMI				3
#define ACPI_MADT_TYPE_LAPIC_NMI			4
#define ACPI_MADT_TYPE_LAPICADDR_OVERRIDE		5
#define ACPI_MADT_TYPE_IOSAPIC				6
#define ACPI_MADT_TYPE_LSAPIC				7
#define ACPI_MADT_TYPE_IRQSOURCE			8


#define ACPI_MADT_CPU_FLAGS_ENABLED			(1<<0)

struct acpi_rMadtCpuS
{
	ubit8		type;
	ubit8		length;
	ubit8		acpiLapicId;
	ubit8		lapicId;
	ubit32		flags;
};


struct acpi_rMadtIoApicS
{
	ubit8		type;
	ubit8		length;
	ubit8		ioApicId;
	ubit8		rsvd;
	ubit32		ioApicPaddr;
	ubit32		globalIrqBase;
};


#define ACPI_MADT_IRQSRCOVER_POLARITY_SHIFT		0x0
#define ACPI_MADT_IRQSRCOVER_POLARITY_MASK		0x3

#define ACPI_MADT_IRQSRCOVER_POLARITY_CONFORMS		0x0
#define ACPI_MADT_IRQSRCOVER_POLARITY_ACTIVEHIGH	0x1
#define ACPI_MADT_IRQSRCOVER_POLARITY_ACTIVELOW		0x3

#define ACPI_MADT_IRQSRCOVER_TRIGGER_SHIFT		0x2
#define ACPI_MADT_IRQSRCOVER_TRIGGER_MASK		0x3

#define ACPI_MADT_IRQSRCOVER_TRIGGER_CONFORMS		0x0
#define ACPI_MADT_IRQSRCOVER_TRIGGER_EDGE		0x1
#define ACPI_MADT_IRQSRCOVER_TRIGGER_LEVEL		0x3

// Struct should be 10B, but alignment causes it to be 12B. Could be an issue.
struct acpi_rMadtIrqSourceOverS
{
	ubit8		type;
	ubit8		length;
	ubit8		bus;
	ubit8		irqNo;
	ubit32		globalIrq;
	ubit16		flags;
} __attribute__((packed));

#define ACPI_MADT_NMI_LAPICID_ALL	0xFF

#define ACPI_MADT_NMI_FLAGS_GET_POLARITY(_f)		((_f) & 0x3)
#define ACPI_MADT_NMI_FLAGS_GET_TRIGGERMODE(_f)		(((_f) >> 2) & 0x3)

struct acpi_rMadtLapicNmiS
{
	ubit8		type;
	ubit8		length;
	ubit8		acpiLapicId;
	ubit16		flags;
	ubit8		lapicLint;
} __attribute__((packed));

#ifdef __cplusplus

namespace acpiRMadt
{
	// Return next LAPIC entry.
	acpi_rMadtCpuS *getNextCpuEntry(acpi_rMadtS *madt, void **const handle);
	acpi_rMadtIoApicS *getNextIoApicEntry(
		acpi_rMadtS *madt, void **const handle);

	acpi_rMadtLapicNmiS *getNextLapicNmiEntry(
		acpi_rMadtS *madt, void **const handle);

	acpi_rMadtIrqSourceOverS *getNextIrqSourceOverrideEntry(
		acpi_rMadtS *madt, void **const handle);
}

namespace acpiXMadt
{
}

#endif

#endif

