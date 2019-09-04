
#include <arch/paddr_t.h>
#include <arch/paging.h>
#include <arch/walkerPageRanger.h>
#include <chipset/findTables.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kcxxlib/memory>
#include <__kclasses/debugPipe.h>
#include <commonlibs/libacpi/rsdp.h>
#include <kernel/common/processTrib/processTrib.h>


static acpi::sSdtCache		cache;

void acpi::initializeCache(void)
{
	if (cache.magic == ACPI_CACHE_MAGIC) { return; };

	acpi::flushCache();
	cache.magic = ACPI_CACHE_MAGIC;
}

void acpi::flushCache(void)
{
	memset(&cache, 0, sizeof(cache));
}

error_t acpi::findRsdp(void)
{
	if (acpi::rsdpFound()) { return ERROR_SUCCESS; };

	// Call on chipset code to find ACPI RSDP.
	cache.rsdp = static_cast<acpi::sRsdp *>( chipset_findAcpiRsdp() );
	if (cache.rsdp == NULL) {
		return ERROR_GENERAL;
	};
	return ERROR_SUCCESS;
}

sarch_t acpi::rsdpFound(void)
{
	if (cache.rsdp != NULL) {
		return 1;
	};
	return 0;
}

acpi::sRsdp *acpi::getRsdp(void)
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

static sarch_t checksumIsValid(acpi::sRsdt *rsdt)
{
	ubit8		checksum=0;
	ubit8		*table=reinterpret_cast<ubit8 *>( rsdt );

	for (ubit32 i=0; i<rsdt->hdr.tableLength; i++) {
		checksum += table[i];
	};

	return (checksum == 0);
}

error_t acpi::mapRsdt(void)
{
	LoosePage<acpi::sRsdt>	rsdt;
	uarch_t			rsdtNPages;

	if (!acpi::rsdpFound() || !acpi::testForRsdt())
		{ return ERROR_GENERAL; };

	if (acpi::getRsdt() != NULL) { return ERROR_SUCCESS; };

	rsdt = (acpi::sRsdt *)walkerPageRanger::createMappingTo(
		cache.rsdp->rsdtPaddr, 2,
		PAGEATTRIB_PRESENT | PAGEATTRIB_SUPERVISOR);

	if (rsdt == NULL)
	{
		printf(ERROR ACPI"Failed to temp map RSDT.\n");
		return ERROR_MEMORY_VIRTUAL_PAGEMAP;
	};

	rsdt.get_deleter().nPages = rsdt.get_deleter().nMapped = 2;
	rsdt.get_deleter().vasStream =
		processTrib.__kgetStream()->getVaddrSpaceStream();

	rsdt = WPRANGER_ADJUST_VADDR(
		rsdt.get(),
		paddr_t(cache.rsdp->rsdtPaddr), acpi::sRsdt *);

	// Ensure that the table is valid: compute checksum.
	if (!checksumIsValid(rsdt.get()))
	{
		printf(WARNING ACPI"RSDT has invalid checksum.\n");
		rsdt = WPRANGER_UNADJUST_VADDR(rsdt.get(), acpi::sRsdt *);
		return ERROR_GENERAL;
	};

	// Find out the RSDT's real size.
	rsdtNPages = PAGING_BYTES_TO_PAGES(rsdt->hdr.tableLength) + 1;
	rsdt = WPRANGER_UNADJUST_VADDR(rsdt.get(), acpi::sRsdt *);
	rsdt.reset();

	// Reallocate vmem.
	rsdt = (acpi::sRsdt *)walkerPageRanger::createMappingTo(
		cache.rsdp->rsdtPaddr, rsdtNPages,
		PAGEATTRIB_PRESENT | PAGEATTRIB_SUPERVISOR);

	if (rsdt == NULL)
	{
		printf(ERROR ACPI"Failed to map RSDT.\n");
		return ERROR_MEMORY_VIRTUAL_PAGEMAP;
	};

	rsdt.get_deleter().nPages = rsdt.get_deleter().nMapped = rsdtNPages;
	rsdt = WPRANGER_ADJUST_VADDR(
		rsdt.get(),
		paddr_t(cache.rsdp->rsdtPaddr), acpi::sRsdt *);

	cache.rsdt = rsdt.release();
	return ERROR_SUCCESS;
}

acpi::sRsdt *acpi::getRsdt(void)
{
	if (!acpi::rsdpFound() || !acpi::testForRsdt()) {
		return NULL;
	};

	return cache.rsdt;
}

