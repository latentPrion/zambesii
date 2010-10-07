#ifndef _ACPI_MADT_H
	#define _ACPI_MADT_H

	#include <__kstdlib/__ktypes.h>
	#include <commonlibs/libacpi/rsdp.h>

#define ACPI_MADT_FLAGS_PC8259_COMPAT		(1<<0)
#define ACPI_MADT_GET_FIRST_ENTRY(_madt)		\
	(void *)((uarch_t)_madt + sizeof(struct acpi_madtS))

#define ACPI_MADT_GET_ENDADDR(_madt)			\
	(void *)((uarch_t)_madt + _madt->hdr.tableLength)
struct acpi_madtS
{
	struct acpi_sdtS	hdr;
	ubit32			lapicPaddr;
	ubit32			flags;
};


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

struct acpi_madtCpuS
{
	ubit8		type;
	ubit8		length;
	ubit8		acpiLapicId;
	ubit8		lapicId;
};


struct acpi_madtIoApicS
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

struct acpi_madtIrqSourceOverS
{
	ubit8		type;
	ubit8		length;
	ubit8		bus;
	ubit8		source;
	ubit32		globalIrq;
	ubit16		flags;
};


struct acpi_madtNmiS
{
	ubit8		type;
	ubit8		length;
	ubit16		flags;
	ubit32		globalIrq;
};


struct acpi_madtLapicNmiS
{
	ubit8		type;
	ubit8		length;
	ubit8		acpiLapicId;
	ubit16		flags;
	ubit8		
#ifdef __cplusplus

namespace acpiMadt
{
	// Return next LAPIC entry.
	acpi_madtCpuS *getNextCpuEntry(acpi_madtS *madt, void **const handle);
}

#endif

#endif

