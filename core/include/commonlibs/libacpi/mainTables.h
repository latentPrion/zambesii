#ifndef _ZBZ_LIB_ACPI_MAIN_TABLES_H
	#define _ZBZ_LIB_ACPI_MAIN_TABLES_H

	#include <__kstdlib/__ktypes.h>
	#include "baseTables.h"

struct acpi_rSratS
{
	struct acpi_sdtS	hdr;
	ubit8			reserved[12];
};

struct acpi_rMadtS
{
};

struct acpi_rFacpS
{
};

#endif

