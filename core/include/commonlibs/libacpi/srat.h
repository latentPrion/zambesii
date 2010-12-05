#ifndef _ZBZ_LIB_ACPI_R_X_SRAT_H
	#define _ZBZ_LIB_ACPI_R_X_SRAT_H

	#include <__kstdlib/__ktypes.h>
	#include "rxsdt.h"

#define ACPI_SRAT_GET_TYPE(_addr)		(*(ubit8 *)_addr)
#define ACPI_SRAT_TYPE_CPU			0
#define ACPI_SRAT_TYPE_MEM			1

#define ACPI_SRAT_CPU_FLAGS_ENABLED		(1<<0)

struct acpi_rSratCpuS
{
	ubit8		type;
	ubit8		length;
	// Domain 0-7.
	ubit8		domain0;
	ubit8		lapicId;
	ubit32		flags;
	// Domain 8-31 + Local SAPIC EID.
	ubit32		domain1;
	ubit8		reserved[4];
};

#define ACPI_SRAT_MEM_FLAGS_ENABLED		(1<<0)
#define ACPI_SRAT_MEM_FLAGS_HOTPLUG		(1<<1)
#define ACPI_SRAT_MEM_FLAGS_NONVOLATILE		(1<<2)

struct acpi_rSratMemS
{
	ubit8		type;
	ubit8		length;
	ubit32		domain;
	ubit16		reserved0;
	ubit32		baseLow;
	ubit32		baseHigh;
	ubit32		lengthLow;
	ubit32		lengthHigh;
	ubit32		reserved1;
	ubit32		flags;
	ubit8		reserved2[8];
};

#ifdef __cplusplus

namespace acpiRSrat
{
	acpi_rSratCpuS *getNextCpuEntry(acpi_rSratS *s, void **const handle);
	acpi_rSratMemS *getNextMemEntry(acpi_rSratS *s, void **const handle);
}

namespace acpiXSrat
{
}

#endif

#endif

