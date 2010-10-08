#ifndef _ZBZ_LIB_ACPI_RSDP_H
	#define _ZBZ_LIB_ACPI_RSDP_H

	#include <__kstdlib/__ktypes.h>
	#include "baseTables.h"


struct acpi_sdtCacheS
{
	acpi_rsdpS	*rsdp;
	acpi_rsdtS	*rsdt;
	acpi_xsdtS	*xsdt;
};

#ifdef __cplusplus

namespace acpi
{
	error_t findRsdp(void);
	sarch_t rsdpFound(void);
	acpi_rsdpS *getRsdp(void);

	sarch_t testForRsdt(void);
	sarch_t testForXsdt(void);

	error_t mapRsdt(void);
	acpi_rsdtS *getRsdt(void);
	error_t mapXsdt(void);
	acpi_xsdtS *getXsdt(void);

	void flushCache(void);
}

#endif

#endif

