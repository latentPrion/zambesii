#ifndef _ZBZ_LIB_ACPI_R_X_SDT_H
	#define _ZBZ_LIB_ACPI_R_X_SDT_H

	#include <__kstdlib/__ktypes.h>
	#include "baseTables.h"
	#include "mainTables.h"


#ifdef __cplusplus

namespace acpiRsdt
{
	// Returns the next SRAT. The lib provides for more than one.
	acpi_rSratS *getNextSrat(
		acpi_rsdtS *r, void **const context, void **const handle);

	// Returns the next MADT. The lib provides for more than one.
	acpi_rMadtS *getNextMadt(
		acpi_rsdtS *r, void **const context, void **const handle);

	// Returns the next FACP.
	acpi_rFacpS *getNextFacp(
		acpi_rsdtS *r, void **const context, void **const handle);

	// Unmaps an ACPI table.
	void destroySdt(acpi_sdtS *sdt);
	// Will *usually* not need to be used since it is called internally.
	void destroyContext(void **const context);
}

namespace acpiXsdt
{
	// Will implement later.
}

#endif

#endif

