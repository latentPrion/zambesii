
#include <arch/paddr_t.h>
#include <arch/paging.h>
#include <arch/walkerPageRanger.h>
#include <chipset/findTables.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kclasses/debugPipe.h>
#include <commonlibs/libacpi/rsdp.h>
#include <kernel/common/memoryTrib/memoryTrib.h>


static acpi_sdtCacheS		cache;

void acpi::initializeCache(void)
{
	if (cache.magic == ACPI_CACHE_MAGIC) { return; };

	acpi::flushCache();
	cache.magic = ACPI_CACHE_MAGIC;
}

void acpi::flushCache(void)
{
	memset8(&cache, 0, sizeof(cache));
}

error_t acpi::findRsdp(void)
{
	if (acpi::rsdpFound()) { return ERROR_SUCCESS; };

	// Call on chipset code to find ACPI RSDP.
	cache.rsdp = static_cast<acpi_rsdpS *>( chipset_findAcpiRsdp() );
	if (cache.rsdp == __KNULL) {
		return ERROR_GENERAL;
	};
	return ERROR_SUCCESS;
}

sarch_t acpi::rsdpFound(void)
{
	if (cache.rsdp != __KNULL) {
		return 1;
	};
	return 0;
}

acpi_rsdpS *acpi::getRsdp(void)
{
	return cache.rsdp;
}

sarch_t acpi::testForRsdt(void)
{
	if (!acpi::rsdpFound()) {
		return 0;
	};

	if (cache.rsdp->rsdtPaddr != 0) {
		return 1;
	};
	return 0;
}

sarch_t acpi::testForXsdt(void)
{
	if (!acpi::rsdpFound()) {
		return 0;
	};

	if (cache.rsdp->xsdtPaddr[0] != 0 || cache.rsdp->xsdtPaddr[1] != 0) {
		return 1;
	};
	return 0;
}

error_t acpi::mapRsdt(void)
{
	acpi_rsdtS	*rsdt;
	status_t	nMapped;
	uarch_t		rsdtNPages, f;
	paddr_t		p;

	if (!acpi::rsdpFound() || !acpi::testForRsdt()) {
		return ERROR_GENERAL;
	};
	if (acpi::getRsdt() != __KNULL) {
		return ERROR_SUCCESS;
	};

	rsdt = new ((memoryTrib.__kmemoryStream.vaddrSpaceStream
		.*memoryTrib.__kmemoryStream.vaddrSpaceStream.getPages)(2))
		acpi_rsdtS;

	if (rsdt == __KNULL) {
		return ERROR_MEMORY_NOMEM_VIRTUAL;
	};

	nMapped = walkerPageRanger::mapInc(
		&memoryTrib.__kmemoryStream.vaddrSpaceStream.vaddrSpace,
		rsdt, cache.rsdp->rsdtPaddr, 2,
		PAGEATTRIB_PRESENT | PAGEATTRIB_SUPERVISOR);

	if (nMapped < 2)
	{
		rsdtNPages = 2;
		goto releaseAndExit;
	};

	rsdt = WPRANGER_ADJUST_VADDR(rsdt, cache.rsdp->rsdtPaddr, acpi_rsdtS *);
	// Find out the RSDT's real size.
	rsdtNPages = PAGING_BYTES_TO_PAGES(rsdt->hdr.tableLength) + 1;

	walkerPageRanger::unmap(
		&memoryTrib.__kmemoryStream.vaddrSpaceStream.vaddrSpace,
		rsdt, &p, 2, &f);

	// Reallocate vmem.
	rsdt = new ((memoryTrib.__kmemoryStream.vaddrSpaceStream
		.*memoryTrib.__kmemoryStream.vaddrSpaceStream.getPages)(2))
		acpi_rsdtS;

	if (rsdt == __KNULL) {
		return ERROR_MEMORY_NOMEM_VIRTUAL;
	};

	nMapped = walkerPageRanger::mapInc(
		&memoryTrib.__kmemoryStream.vaddrSpaceStream.vaddrSpace,
		rsdt, cache.rsdp->rsdtPaddr, rsdtNPages,
		PAGEATTRIB_PRESENT | PAGEATTRIB_SUPERVISOR);

	if (nMapped < static_cast<status_t>( rsdtNPages )) {
		goto releaseAndExit;
	};

	rsdt = WPRANGER_ADJUST_VADDR(rsdt, cache.rsdp->rsdtPaddr, acpi_rsdtS *);
	cache.rsdt = rsdt;
	return ERROR_SUCCESS;

releaseAndExit:
	walkerPageRanger::unmap(
		&memoryTrib.__kmemoryStream.vaddrSpaceStream.vaddrSpace,
		rsdt, &p, rsdtNPages, &f);

	memoryTrib.__kmemoryStream.vaddrSpaceStream.releasePages(
		rsdt, rsdtNPages);

	return ERROR_MEMORY_VIRTUAL_PAGEMAP;
}

acpi_rsdtS *acpi::getRsdt(void)
{
	if (!acpi::rsdpFound() || !acpi::testForRsdt()) {
		return __KNULL;
	};

	return cache.rsdt;
}

