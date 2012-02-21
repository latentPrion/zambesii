
#include <arch/walkerPageRanger.h>
#include <arch/paging.h>
#include <chipset/findTables.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kcxxlib/new>
#include <commonlibs/libx86mp/mpTables.h>
#include <commonlibs/libx86mp/mpDefaultTables.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/memoryTrib/memoryTrib.h>


static struct x86_mpCacheS	cache;

void x86Mp::initializeCache(void)
{
	// Don't zero out the cache while it's in use.
	if (cache.magic == x86_MPCACHE_MAGIC) {
		return;
	};

	x86Mp::flushCache();
	cache.magic = x86_MPCACHE_MAGIC;
}

void x86Mp::flushCache(void)
{
	memset8(&cache, 0, sizeof(cache));
}

x86_mpFpS *x86Mp::findMpFp(void)
{
	x86_mpFpS	*ret;

	/* This function must call on the chipset for help. The chipset must
	 * scan for the MP FP structure in its own chipset specific manner.
	 *
	 * This function should not fail if there is MP on the board.
	 **/
	if (cache.fp != __KNULL) {
		return cache.fp;
	};

	ret = (struct x86_mpFpS *)chipset_findx86MpFp();
	if (ret != __KNULL) {
		cache.fp = ret;
	}

	return ret;
}

sarch_t x86Mp::mpFpFound(void)
{
	if (cache.magic != x86_MPCACHE_MAGIC) {
		return 0;
	};

	if (cache.fp == __KNULL) {
		return 0;
	};

	return 1;
}

static sarch_t checksumIsValid(x86_mpCfgS *cfg)
{
	ubit8		checksum=0;
	ubit8		*table=reinterpret_cast<ubit8 *>( cfg );

	for (ubit16 i=0; i<cfg->length; i++) {
		checksum += table[i];
	};

	return (checksum == 0) ? 1 : 0;
}

x86_mpCfgS *x86Mp::mapMpConfigTable(void)
{
	ubit32		cfgPaddr;
	ubit32		cfgNPages;
	x86_mpCfgS	*ret;

	if (!x86Mp::mpFpFound()) {
		return __KNULL;
	};

	if (cache.cfg != __KNULL) {
		return cache.cfg;
	};

	// See if we need to return a default config.
	if (cache.fp->features[0] != 0)
	{
		__kprintf(NOTICE x86MP"MP FP indicates default config %d.\n",
			cache.fp->features[0]);

		cache.cfg = x86_mpCfgDefaults[cache.fp->features[0]];
		cache.defaultConfig = cache.fp->features[0];
		cache.nCfgEntries = cache.cfg->nEntries;
		cache.lapicPaddr = cache.cfg->lapicPaddr;
		return cache.cfg;
	};

	cfgPaddr = cache.fp->cfgTablePaddr;
	ret = (x86_mpCfgS *)walkerPageRanger::createMappingTo(
		cfgPaddr, 1, PAGEATTRIB_PRESENT | PAGEATTRIB_SUPERVISOR);

	if (ret == __KNULL)
	{
		__kprintf(ERROR x86MP"mapMpCfgTable: Failed to map.\n");
		return __KNULL;
	};

	ret = WPRANGER_ADJUST_VADDR(ret, cache.fp->cfgTablePaddr, x86_mpCfgS *);
	cfgNPages = PAGING_BYTES_TO_PAGES(ret->length) + 1;

	// First ensure that the table's checksum is valid.
	if (!checksumIsValid(ret))
	{
		__kprintf(WARNING x86MP"mapMpCfgTable: Invalid checksum.\n");
		memoryTrib.__kmemoryStream.vaddrSpaceStream.releasePages(
			ret, 1);

		return __KNULL;
	};

	// Free the temporary mapping and its mem, then re-map the cfg table.
	memoryTrib.__kmemoryStream.vaddrSpaceStream.releasePages(
		(void *)((uarch_t)ret & PAGING_BASE_MASK_HIGH), 1);

	ret = (x86_mpCfgS *)walkerPageRanger::createMappingTo(
		cfgPaddr, cfgNPages,
		PAGEATTRIB_PRESENT | PAGEATTRIB_SUPERVISOR);

	if (ret == __KNULL)
	{
		__kprintf(ERROR x86MP"mapMpCfgTable: Failed to map.\n");
		return __KNULL;
	};

	ret = WPRANGER_ADJUST_VADDR(ret, cache.fp->cfgTablePaddr, x86_mpCfgS *);
	cache.cfg = ret;
	cache.lapicPaddr = cache.cfg->lapicPaddr;
	cache.nCfgEntries = ret->nEntries;

	__kprintf(NOTICE x86MP"Mapped MP Config table to 0x%X, %d pages. %d "
		"entries in MP config.\n",
		ret, cfgNPages, ret->nEntries);

	// Now we can return the table vaddr.
	return ret;
}

void x86Mp::unmapMpConfigTable(void)
{
	uarch_t		nPages, f;
	paddr_t		p;

	if (!mpFpFound() || cache.cfg == __KNULL || cache.fp->features[0] != 0)
	{
		return;
	};

	// Find out how many pages to unmap, and proceed.
	nPages = PAGING_BYTES_TO_PAGES(cache.cfg->length) + 1;
	walkerPageRanger::unmap(
		&memoryTrib.__kmemoryStream.vaddrSpaceStream.vaddrSpace,
		cache.cfg, &p, nPages, &f);

	memoryTrib.__kmemoryStream.vaddrSpaceStream.releasePages(
		cache.cfg, nPages);

	cache.cfg = __KNULL;
}

sarch_t x86Mp::mpConfigTableIsMapped(void)
{
	if (!mpFpFound() || cache.cfg == __KNULL) {
		return 0;
	};

	return 1;
}

status_t x86Mp::getChipsetDefaultConfig(void)
{
	/* Determines whether or not the chipset uses an MP default config.
	 * Returns negative value if the cache is uninitialized, 0 if the
	 * chipset provides its own config table, and returns value greater
	 * than 0 (specifically the default config number) if yes.
	 *
	 * The value returned, if greater than 0, is an index into a hardcoded
	 * table of CPU Config and CPU Maps to return for each default config.
	 **/
	if (!cache.magic == x86_MPCACHE_MAGIC || !x86Mp::mpFpFound()) {
		return -1;
	};

	return cache.fp->features[0];
}

ubit32 x86Mp::getLapicPaddr(void)
{
	if (x86Mp::getMpCfg() == __KNULL) {
		return 0;
	};

	return cache.lapicPaddr;
}

x86_mpFpS *x86Mp::getMpFp(void)
{
	if (cache.magic != x86_MPCACHE_MAGIC) {
		return __KNULL;
	};

	return cache.fp;
}

x86_mpCfgS *x86Mp::getMpCfg(void)
{
	if (!x86Mp::mpFpFound()) {
		return __KNULL;
	};

	return x86Mp::mapMpConfigTable();
}

x86_mpCfgCpuS *x86Mp::getNextCpuEntry(uarch_t *pos, void **const handle)
{
	x86_mpCfgCpuS	*ret=__KNULL;

	if (!x86Mp::getMpCfg()) {
		return __KNULL;
	};

	if (*pos == 0)
	{
		// Caller wants a fresh iteration.
		*handle = (void *)((uarch_t)cache.cfg + sizeof(x86_mpCfgS));
	};

	for (; *pos < cache.nCfgEntries; *pos += 1)
	{
		if (ret != __KNULL) {
			break;
		};

		if (*((ubit8 *)*handle) == x86_MPCFG_TYPE_CPU) {
			ret = (x86_mpCfgCpuS *)*handle;
		};

		switch (*(ubit8 *)*handle)
		{
		case x86_MPCFG_TYPE_CPU:
			*handle = (void *)(
				(uarch_t)*handle + sizeof(x86_mpCfgCpuS));

			break;

		case x86_MPCFG_TYPE_BUS:
			*handle = (void *)(
				(uarch_t)*handle + sizeof(x86_mpCfgBusS));

			break;

		case x86_MPCFG_TYPE_IOAPIC:
			*handle = (void *)(
				(uarch_t)*handle + sizeof(x86_mpCfgIoApicS));

			break;

		case x86_MPCFG_TYPE_IRQSOURCE:
			*handle = (void *)(
				(uarch_t)*handle + sizeof(x86_mpCfgIrqSourceS));

			break;

		case x86_MPCFG_TYPE_LOCALIRQSOURCE:
			*handle = (void *)(
				(uarch_t)*handle
				+ sizeof(x86_mpCfgLocalIrqSourceS));

			break;

		default: // This should NEVER be reached.
			__kprintf(ERROR x86MP"Encountered CFG entry with "
				"unknown type 0x%X. Ending loop.\n",
				*(ubit8 *)*handle);

			return __KNULL;
		};
	};

	return ret;
}

x86_mpCfgIoApicS *x86Mp::getNextIoApicEntry(uarch_t *pos, void **const handle)
{
	x86_mpCfgIoApicS	*ret=__KNULL;

	if (!x86Mp::getMpCfg()) {
		return __KNULL;
	};

	if (*pos == 0)
	{
		// Caller wants a fresh iteration.
		*handle = (void *)((uarch_t)cache.cfg + sizeof(x86_mpCfgS));
	};

	for (; *pos < cache.nCfgEntries; *pos += 1)
	{
		if (ret != __KNULL) {
			break;
		};

		if (*((ubit8 *)*handle) == x86_MPCFG_TYPE_IOAPIC) {
			ret = (x86_mpCfgIoApicS *)*handle;
		};

		switch (*(ubit8 *)*handle)
		{
		case x86_MPCFG_TYPE_CPU:
			*handle = (void *)(
				(uarch_t)*handle + sizeof(x86_mpCfgCpuS));

			break;

		case x86_MPCFG_TYPE_BUS:
			*handle = (void *)(
				(uarch_t)*handle + sizeof(x86_mpCfgBusS));

			break;

		case x86_MPCFG_TYPE_IOAPIC:
			*handle = (void *)(
				(uarch_t)*handle + sizeof(x86_mpCfgIoApicS));

			break;

		case x86_MPCFG_TYPE_IRQSOURCE:
			*handle = (void *)(
				(uarch_t)*handle + sizeof(x86_mpCfgIrqSourceS));

			break;

		case x86_MPCFG_TYPE_LOCALIRQSOURCE:
			*handle = (void *)(
				(uarch_t)*handle
				+ sizeof(x86_mpCfgLocalIrqSourceS));

			break;

		default: // This should NEVER be reached.
			__kprintf(ERROR x86MP"Encountered CFG entry with "
				"unknown type 0x%X. Ending loop.\n",
				*(ubit8 *)*handle);

			return __KNULL;
		};
	};

	return ret;
}

x86_mpCfgLocalIrqSourceS *x86Mp::getNextLocalIrqSourceEntry(
	uarch_t *pos, void **const handle
	)
{
	x86_mpCfgLocalIrqSourceS	*ret=__KNULL;

	if (!x86Mp::getMpCfg()) {
		return __KNULL;
	};

	if (*pos == 0)
	{
		// Caller wants a fresh iteration.
		*handle = (void *)((uarch_t)cache.cfg + sizeof(x86_mpCfgS));
	};

	for (; *pos < cache.nCfgEntries; *pos += 1)
	{
		if (ret != __KNULL) {
			break;
		};

		if (*((ubit8 *)*handle) == x86_MPCFG_TYPE_LOCALIRQSOURCE) {
			ret = (x86_mpCfgLocalIrqSourceS *)*handle;
		};

		switch (*(ubit8 *)*handle)
		{
		case x86_MPCFG_TYPE_CPU:
			*handle = (void *)(
				(uarch_t)*handle + sizeof(x86_mpCfgCpuS));

			break;

		case x86_MPCFG_TYPE_BUS:
			*handle = (void *)(
				(uarch_t)*handle + sizeof(x86_mpCfgBusS));

			break;

		case x86_MPCFG_TYPE_IOAPIC:
			*handle = (void *)(
				(uarch_t)*handle + sizeof(x86_mpCfgIoApicS));

			break;

		case x86_MPCFG_TYPE_IRQSOURCE:
			*handle = (void *)(
				(uarch_t)*handle + sizeof(x86_mpCfgIrqSourceS));

			break;

		case x86_MPCFG_TYPE_LOCALIRQSOURCE:
			*handle = (void *)(
				(uarch_t)*handle
				+ sizeof(x86_mpCfgLocalIrqSourceS));

			break;

		default: // This should NEVER be reached.
			__kprintf(ERROR x86MP"Encountered CFG entry with "
				"unknown type 0x%X. Ending loop.\n",
				*(ubit8 *)*handle);

			return __KNULL;
		};
	};

	return ret;
}

