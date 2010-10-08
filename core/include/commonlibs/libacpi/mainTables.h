#ifndef _ZBZ_LIB_ACPI_MAIN_TABLES_H
	#define _ZBZ_LIB_ACPI_MAIN_TABLES_H

	#include <__kstdlib/__ktypes.h>
	#include "baseTables.h"

struct acpi_rSratS
{
	struct acpi_sdtS	hdr;
	ubit8			reserved[12];
};

#define ACPI_MADT_FLAGS_PC8259_COMPAT		(1<<0)
#define ACPI_MADT_GET_FIRST_ENTRY(_madt)		\
	(void *)((uarch_t)_madt + sizeof(struct acpi_rMadtS))

#define ACPI_MADT_GET_ENDADDR(_madt)			\
	(void *)((uarch_t)_madt + _madt->hdr.tableLength)

struct acpi_rMadtS
{
	struct acpi_sdtS	hdr;
	ubit32			lapicPaddr;
	ubit32			flags;
};

struct acpi_rFacpS
{
};

#endif

