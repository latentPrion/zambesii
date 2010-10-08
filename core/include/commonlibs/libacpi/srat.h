#ifndef _ZBZ_LIB_ACPI_R_X_SRAT_H
	#define _ZBZ_LIB_ACPI_R_X_SRAT_H

	#include <__kstdlib/__ktypes.h>
	#include "rxsdt.h"

namespace acpiRSrat
{
	acpi_rSratCpuS *getNextCpuEntry(acpi_rSratS *s, void **const handle);
	acpi_rSratIoApicS *getNextIoApicEntry(
		acpi_rSratS *s, void **const handle);
}

namespace acpiXSrat
{
}

#endif

