#ifndef _ZBZ_LIB_ACPI_RSDP_H
	#define _ZBZ_LIB_ACPI_RSDP_H

	#include <__kstdlib/__ktypes.h>
	#include "baseTables.h"


// "ACPI OK"
#define ACPI			"ACPI: "
#define ACPI_CACHE_MAGIC	0xAC1D101C

#ifdef __cplusplus

namespace acpi
{
	struct sSdtCache
	{
		ubit32		magic;
		acpi::sRsdp	*rsdp;
		acpi::sRsdt	*rsdt;
		acpi::sXsdt	*xsdt;
	};

	// Call before using, every time.
	void initializeCache(void);

	// TODO: Remember to remove.
	void debug(void);
	void *cacheVaddr(void);

	error_t findRsdp(void);
	sarch_t rsdpFound(void);
	acpi::sRsdp *getRsdp(void);

	sarch_t testForRsdt(void);
	sarch_t testForXsdt(void);

	error_t mapRsdt(void);
	acpi::sRsdt *getRsdt(void);
	error_t mapXsdt(void);
	acpi::sXsdt *getXsdt(void);

	void flushCache(void);
}

#endif

#endif

