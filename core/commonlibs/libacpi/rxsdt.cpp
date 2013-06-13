
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

	if (*context == __KNULL)
	{
		*context = processTrib.__kgetStream()->getVaddrSpaceStream()
			->getPages(2);

		if (*context == __KNULL) {
			return __KNULL;
		};
	};

	nMapped = walkerPageRanger::mapInc(
		&processTrib.__kgetStream()->getVaddrSpaceStream()->vaddrSpace,
		*context, p, 2,
		PAGEATTRIB_PRESENT | PAGEATTRIB_SUPERVISOR);

	if (nMapped < 2) {
		return __KNULL;
	};

	*context = WPRANGER_ADJUST_VADDR((*context), p, void *);
	return *context;
}

void acpiRsdt::destroyContext(void **const context)
{
	paddr_t		p;
	uarch_t		f;

	if (*context == __KNULL) { return; };

	walkerPageRanger::unmap(
		&processTrib.__kgetStream()->getVaddrSpaceStream()->vaddrSpace,
		*context, &p, 2, &f);

	processTrib.__kgetStream()->getVaddrSpaceStream()->releasePages(
		(void *)((uintptr_t)(*context) & PAGING_BASE_MASK_HIGH), 2);

	*context = __KNULL;
}

static void *acpi_mapTable(paddr_t p, uarch_t nPages)
{
	void		*ret;
	status_t	nMapped;

	ret = processTrib.__kgetStream()->getVaddrSpaceStream()->getPages(
		nPages);

	if (ret == __KNULL) {
		return __KNULL;
	};

	nMapped = walkerPageRanger::mapInc(
		&processTrib.__kgetStream()->getVaddrSpaceStream()->vaddrSpace,
		ret, p, nPages,
		PAGEATTRIB_PRESENT | PAGEATTRIB_SUPERVISOR);

	if (nMapped < static_cast<status_t>( nPages ))
	{
		processTrib.__kgetStream()->getVaddrSpaceStream()->releasePages(
			ret, nPages);

		return __KNULL;
	};

	ret = WPRANGER_ADJUST_VADDR(ret, p, void *);
	return ret;
}

static sarch_t checksumIsValid(acpi_sdtS *sdt)
{
	ubit8		checksum=0;
	ubit8		*table=reinterpret_cast<ubit8 *>( sdt );

	for (ubit32 i=0; i<sdt->tableLength; i++) {
		checksum += table[i];
	};

	return (checksum == 0);
}


acpi_rSratS *acpiRsdt::getNextSrat(
	acpi_rsdtS *rsdt, void **const context, void **const handle
	)
{
	acpi_sdtS	*sdt;
	acpi_rSratS	*ret=__KNULL;

	if (*handle == __KNULL) {
		*handle = ACPI_TABLE_GET_FIRST_ENTRY(rsdt);
	};

	for (; *handle < ACPI_TABLE_GET_ENDADDR(rsdt); )
	{
		sdt = (acpi_sdtS *)acpi_tmpMapSdt(context, *(paddr_t *)*handle);

		if ((strncmp8(sdt->sig, ACPI_SDT_SIG_SRAT, 4) == 0)
			&& checksumIsValid(sdt))
		{
			ret = (acpi_rSratS *)acpi_mapTable(
				*(paddr_t *)*handle,
				PAGING_BYTES_TO_PAGES(sdt->tableLength) + 1);
		};

		if ((strncmp8(sdt->sig, ACPI_SDT_SIG_SRAT, 4) == 0)
			&& (!checksumIsValid(sdt)))
		{
			__kprintf(WARNING ACPIR"SRAT with invalid checksum @P "
				"0x%P.\n",
				*handle);
		};

		*handle = reinterpret_cast<void *>( (uarch_t)*handle + 4 );
		if (ret != __KNULL) {
			return ret;
		};
	};

	destroyContext(context);
	return ret;
}

acpi_rMadtS *acpiRsdt::getNextMadt(
	acpi_rsdtS *rsdt, void **const context, void **const handle
	)
{
	acpi_sdtS	*sdt;
	acpi_rMadtS	*ret=__KNULL;

	if (*handle == __KNULL) {
		*handle = ACPI_TABLE_GET_FIRST_ENTRY(rsdt);
	};

	for (; *handle < ACPI_TABLE_GET_ENDADDR(rsdt); )
	{
		sdt = (acpi_sdtS *)acpi_tmpMapSdt(context, *(paddr_t *)*handle);
		if ((strncmp8(sdt->sig, ACPI_SDT_SIG_APIC, 4) == 0)
			&& checksumIsValid(sdt))
		{
			ret = (acpi_rMadtS *)acpi_mapTable(
				*(paddr_t *)*handle,
				PAGING_BYTES_TO_PAGES(sdt->tableLength) + 1);
		};

		if ((strncmp8(sdt->sig, ACPI_SDT_SIG_APIC, 4) == 0)
			&& (!checksumIsValid(sdt)))
		{
			__kprintf(WARNING ACPIR"MADT with invalid checksum @P "
				"0x%P.\n",
				*handle);
		};

		*handle = reinterpret_cast<void *>( (uarch_t)*handle + 4 );
		if (ret != __KNULL) {
			return ret;
		};
	};

	destroyContext(context);
	return ret;
}

acpi_rFadtS *acpiRsdt::getNextFadt(
	acpi_rsdtS *rsdt, void **const context, void **const handle
	)
{
	acpi_sdtS	*sdt;
	acpi_rFadtS	*ret=__KNULL;

	if (*handle == __KNULL) {
		*handle = ACPI_TABLE_GET_FIRST_ENTRY(rsdt);
	};

	for (; *handle < ACPI_TABLE_GET_ENDADDR(rsdt); )
	{
		sdt = (acpi_sdtS *)acpi_tmpMapSdt(context, *(paddr_t *)*handle);
		if ((strncmp8(sdt->sig, ACPI_SDT_SIG_FACP, 4) == 0)
			&& checksumIsValid(sdt))
		{
			ret = (acpi_rFadtS *)acpi_mapTable(
				*(paddr_t *)*handle,
				PAGING_BYTES_TO_PAGES(sdt->tableLength) + 1);
		};

		if ((strncmp8(sdt->sig, ACPI_SDT_SIG_FACP, 4) == 0)
			&& (!checksumIsValid(sdt)))
		{
			__kprintf(WARNING ACPIR"FADT with invalid checksum @P "
				"0x%P.\n",
				*handle);
		};

		*handle = reinterpret_cast<void *>( (uarch_t)*handle + 4 );
		if (ret != __KNULL) {
			return ret;
		};
	};

	destroyContext(context);
	return ret;
}

#include <kernel/common/cpuTrib/cpuTrib.h>
void acpiRsdt::destroySdt(acpi_sdtS *sdt)
{
	uarch_t		nPages, f;
	paddr_t		p;

	if (sdt == __KNULL) {
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
if (!__KFLAG_TEST(cpuTrib.getCurrentCpuStream()->flags, CPUSTREAM_FLAGS_BSP)) {__kprintf(NOTICE"Destroying SDT @v 0x%p on CPU %d.\n", sdt, cpuTrib.getCurrentCpuStream()->cpuId);};
}

