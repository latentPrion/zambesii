#ifndef _ZBZ_LIB_ACPI_R_X_SRAT_H
	#define _ZBZ_LIB_ACPI_R_X_SRAT_H

	#include <__kstdlib/__ktypes.h>
	#include "rxsdt.h"

#define ACPI_SRAT_GET_TYPE(_addr)		(*(ubit8 *)(_addr))
#define ACPI_SRAT_TYPE_CPU			0
#define ACPI_SRAT_TYPE_MEM			1

namespace acpiR
{
	namespace srat
	{
#ifdef __cplusplus
		struct sCpu;
		struct sMem;
		sCpu *getNextCpuEntry(acpiR::sSrat *s, void **const handle);
		sMem *getNextMemEntry(acpiR::sSrat *s, void **const handle);
#endif

		#define ACPI_SRAT_CPU_FLAGS_ENABLED		(1<<0)

		struct sCpu
		{
			ubit8		type;
			ubit8		length;
			// Domain 0-7.
			ubit8		domain0;
			ubit8		lapicId;
			ubit32		flags;
			// Domain 8-31 + Local SAPIC EID.
			ubit8		domain1;
			ubit8		localSapicEid;
			ubit8		reserved[4];
		};

		#define ACPI_SRAT_MEM_FLAGS_ENABLED		(1<<0)
		#define ACPI_SRAT_MEM_FLAGS_HOTPLUG		(1<<1)
		#define ACPI_SRAT_MEM_FLAGS_NONVOLATILE		(1<<2)

		struct sMem
		{
			ubit8		type;
			ubit8		length;
			ubit16		domain0;
			ubit16		domain1;
			ubit16		reserved0;
			ubit32		baseLow;
			ubit32		baseHigh;
			ubit32		lengthLow;
			ubit32		lengthHigh;
			ubit32		reserved1;
			ubit32		flags;
			ubit8		reserved2[8];
		};
	}
}

#endif

