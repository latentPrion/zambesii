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

struct acpi_rMadtIrqSourceOverS
{
	ubit8		type;
	ubit8		length;
	ubit8		bus;
	ubit8		source;
	ubit32		globalIrq;
	ubit16		flags;
};


struct acpi_rMadtNmiS
{
	ubit8		type;
	ubit8		length;
	ubit16		flags;
	ubit32		globalIrq;
};


struct acpi_rMadtLapicNmiS
{
	ubit8		pad[6];
};

struct acpi_rMadtLapicPaddrOverS
{
	ubit8		pad[12];
};

struct acpi_rMadtIoSapicS
{
	ubit8		pad[16];
};

struct acpi_rMadtLSapicS
{
	ubit8		pad[16];
};

struct acpi_rMadtIrqSourceS
{
	ubit8		pad[16];
};

	
#ifdef __cplusplus

namespace acpiRMadt
{
	// Return next LAPIC entry.
	acpi_rMadtCpuS *getNextCpuEntry(acpi_rMadtS *madt, void **const handle);
	acpi_rMadtIoApicS *getNextIoApicEntry(
		acpi_rMadtS *madt, void **const handle);
}

namespace acpiXMadt
{
}

#endif

#endif

