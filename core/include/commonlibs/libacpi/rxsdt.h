#ifndef _ZBZ_LIB_ACPI_R_X_SDT_H
	#define _ZBZ_LIB_ACPI_R_X_SDT_H

	#include <__kstdlib/__ktypes.h>
	#include "baseTables.h"
	#include "mainTables.h"

#define ACPIR			"ACPI-RSDT: "
#ifdef __cplusplus

namespace acpiRsdt
{
	// Returns the next SRAT. The lib provides for more than one.
	acpi_rSratS *getNextSrat(
		acpi_sRsdt *r, void **const context, void **const handle);

	// Returns the next MADT. The lib provides for more than one.
	acpi_rMadtS *getNextMadt(
		acpi_sRsdt *r, void **const context, void **const handle);

	// Returns the next FACP.
	acpi_rFadtS *getNextFadt(
		acpi_sRsdt *r, void **const context, void **const handle);

	/**	EXPLANATION:
	 * These two functions are the garbage collection for the lib.
	 * (*) destroySdt():
	 *	This is used to unmap an ACPI table returned by any of the
	 *	functions getNextMadt(), getNextSrat(), getNextFacp() etc
	 *	defined above. Always use this when done cycling through the
	 *	RSDT, whether or not you let the lib return multiple instances
	 *	of the same table if a BIOS like that exists.
	 *
	 * (*) destroyContext():
	 *	While the lib is walking through the RSDT, it demand allocates
	 *	2 pages worth of loose vmem and maps them to the addresses of
	 *	tables in the RSDT. Of course, each physical address needs to be
	 *	mapped one by one, and the signature (FACP, SRAT, etc) needs to
	 *	be checked to see if it matches the type of table being looked
	 *	for by the caller.
	 *
	 *	The "context" argument is used to hold these two temporary,
	 *	loose pages which are demand mapped repeatedly to new tables for
	 *	signature checking. destroyContext() is called internally by the
	 *	lib at the end of a run through the RSDT.
	 *
	 *	This implies that you ran through the RSDT wholly, and you
	 *	were interested in processing multiple instances of a single
	 *	table type. For example, if an errant BIOS had generated 2 MADTs
	 *	this lib is able to process as many as there are. The
	 *	caller may however choose to only process the first one that
	 *	comes up. If you did NOT loop through the whole table, and only
	 *	processed the first one that was returned, then you must call
	 *	destroyContext() since the lib internals only call
	 *	destroyContext() if a full cycle-through was completed.
	 *
	 *	It should be called right after you call destroySdt().
	 **/
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

