
#include <debug.h>
#include <__kclasses/debugPipe.h>

#include <arch/paddr_t.h>
#include <arch/paging.h>
#include <arch/walkerPageRanger.h>
#include <__kstdlib/__kclib/string8.h>
#include <commonlibs/libacpi/rxsdt.h>
#include <commonlibs/libacpi/facp.h>
#include <kernel/common/processTrib/processTrib.h>


static void *acpi_tmpMapSdt(void **const context, paddr_t p)
{
	status_t	nMapped;

	if (*context == NULL)
	{
		*context = processTrib.__kgetStream()->getVaddrSpaceStream()
			->getPages(2);

		if (*context == NULL) {
			return NULL;
		};
	};

	nMapped = walkerPageRanger::mapInc(
		&processTrib.__kgetStream()->getVaddrSpaceStream()->vaddrSpace,
		*context, p, 2,
		PAGEATTRIB_PRESENT | PAGEATTRIB_SUPERVISOR);

	if (nMapped < 2) {
		return NULL;
	};

	*context = WPRANGER_ADJUST_VADDR((*context), p, void *);
	return *context;
}

void acpiRsdt::destroyContext(void **const context)
{
	paddr_t		p;
	uarch_t		f;

	if (*context == NULL) { return; };

	walkerPageRanger::unmap(
		&processTrib.__kgetStream()->getVaddrSpaceStream()->vaddrSpace,
		*context, &p, 2, &f);

	processTrib.__kgetStream()->getVaddrSpaceStream()->releasePages(
		(void *)((uintptr_t)(*context) & PAGING_BASE_MASK_HIGH), 2);

	*context = NULL;
}

static void *acpi_mapTable(paddr_t p, uarch_t nPages)
{
	void		*ret;
	status_t	nMapped;

	ret = processTrib.__kgetStream()->getVaddrSpaceStream()->getPages(
		nPages);

	if (ret == NULL) {
		return NULL;
	};

	nMapped = walkerPageRanger::mapInc(
		&processTrib.__kgetStream()->getVaddrSpaceStream()->vaddrSpace,
		ret, p, nPages,
		PAGEATTRIB_PRESENT | PAGEATTRIB_SUPERVISOR);

	if (nMapped < static_cast<status_t>( nPages ))
	{
		processTrib.__kgetStream()->getVaddrSpaceStream()->releasePages(
			ret, nPages);

		return NULL;
	};

	ret = WPRANGER_ADJUST_VADDR(ret, p, void *);
	return ret;
}

static sarch_t checksumIsValid(acpi::sSdt *sdt)
{
	ubit8		checksum=0;
	ubit8		*table=reinterpret_cast<ubit8 *>( sdt );

	for (ubit32 i=0; i<sdt->tableLength; i++) {
		checksum += table[i];
	};

	return (checksum == 0);
}

static acpi::sSdt *acpi_getNextTable(
	acpi::sRsdt *rsdt,
	void **const context,
	void **const handle,
	const utf8Char *sig,
	const utf8Char *tableName
	)
{
	acpi::sSdt	*sdt;

	if (*handle == NULL) {
		*handle = ACPI_TABLE_GET_FIRST_ENTRY(rsdt);
	};

	for (; *handle < ACPI_TABLE_GET_ENDADDR(rsdt); )
	{
		sdt = (acpi::sSdt *)acpi_tmpMapSdt(context, *(paddr_t *)*handle);

		if ((strncmp8(sdt->sig, sig, 4) == 0) && checksumIsValid(sdt))
		{
			return (acpi::sSdt *)acpi_mapTable(
				*(paddr_t *)*handle,
				PAGING_BYTES_TO_PAGES(sdt->tableLength) + 1);
		};

		if ((strncmp8(sdt->sig, sig, 4) == 0) && (!checksumIsValid(sdt)))
		{
			printf(WARNING ACPIR"%s with invalid checksum @P %P.\n",
				tableName, *handle);
		};

		*handle = reinterpret_cast<void *>( (uarch_t)*handle + 4 );
	};

	destroyContext(context);
	return NULL;
}


acpiR::sSrat *acpiRsdt::getNextSrat(
	acpi::sRsdt *rsdt, void **const context, void **const handle
	)
{
	return (acpiR::sSrat *)acpi_getNextTable(
		rsdt, context, handle, ACPI_SDT_SIG_SRAT, CC"SRAT");
}

acpiR::sMadt *acpiRsdt::getNextMadt(
	acpi::sRsdt *rsdt, void **const context, void **const handle
	)
{
	return (acpiR::sMadt *)acpi_getNextTable(
		rsdt, context, handle, ACPI_SDT_SIG_APIC, CC"MADT");
}

acpiR::sFadt *acpiRsdt::getNextFadt(
	acpi::sRsdt *rsdt, void **const context, void **const handle
	)
{
	return (acpiR::sFadt *)acpi_getNextTable(
		rsdt, context, handle, ACPI_SDT_SIG_FACP, CC"FADT");
}

#include <kernel/common/cpuTrib/cpuTrib.h>
void acpiRsdt::destroySdt(acpi::sSdt *sdt)
{
	uarch_t		nPages, f;
	paddr_t		p;

	if (sdt == NULL) {
		return;
	};

	// Find out how many pages:
	nPages = PAGING_BYTES_TO_PAGES(sdt->tableLength) + 1;
	walkerPageRanger::unmap(
		&processTrib.__kgetStream()->getVaddrSpaceStream()->vaddrSpace,
		sdt, &p, nPages, &f);

	processTrib.__kgetStream()->getVaddrSpaceStream()->releasePages(
		(void *)((uintptr_t)sdt & PAGING_BASE_MASK_HIGH),
		nPages);
if (!cpuTrib.getCurrentCpuStream()->isBspCpu()) {printf(NOTICE"Destroying SDT @v %p on CPU %d.\n", sdt, cpuTrib.getCurrentCpuStream()->cpuId);};
}
