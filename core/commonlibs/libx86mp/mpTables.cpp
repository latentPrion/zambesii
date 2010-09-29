
#include <commonlibs/libx86mp/mpTables.h>
#include <arch/walkerPageRanger.h>
#include <arch/paging.h>
#include <chipset/findTables.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <kernel/common/memoryTrib/memoryTrib.h>


static struct x86_mpCacheS	cache;

void x86Mp::initializeCache(void)
{
	// Don't zero out the cache while it's in use.
	if (cache.isInitialized == 1) {
		return;
	};

	memset(&cache, 0, sizeof(cache));
	cache.isInitialized = 1;
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

sarch_t x86Mp::mpTablesFound(void)
{
	if (cache.isInitialized == 0) {
		return 0;
	};

	if (cache.fp == __KNULL) {
		return 0;
	};

	return 1;
}

x86_mpCfgS *x86Mp::mapMpConfigTable(void)
{
	ubit32		cfgPaddr;
	ubit32		cfgNPages;
	status_t	nMapped;
	x86_mpCfgS	*ret;

	if (!x86Mp::mpTablesFound()) {
		return __KNULL;
	};

	cfgPaddr = cache.fp->cfgTablePaddr;
	// Map the cfg table into the kernel vaddrspace.
	ret = new ((memoryTrib.__kmemoryStream.vaddrSpaceStream
		.*memoryTrib.__kmemoryStream.vaddrSpaceStream.getPages)(1))
		x86_mpCfgS;

	nMapped = walkerPageRanger::mapInc(
		&memoryTrib.__kmemoryStream.vaddrSpaceStream.vaddrSpace,
		ret, cfgPaddr, 1,
		// Don't need write perms here.
		PAGEATTRIB_PRESENT | PAGEATTRIB_SUPERVISOR);

	if (nMapped < 1)
	{
		__kprintf(ERROR x86MP"Failed to map MP Config Table.\n");
		return __KNULL;
	};

	ret = (x86_mpCfgS *)(
		(uarch_t)ret + (cache.fp->cfgTablePaddr
			& PAGING_BASE_MASK_LOW));

	cfgNPages = PAGING_BYTES_TO_PAGES(ret->length) + 1;

	// Free cfg table mem, and reallocate enough to hold the whole thing.
	memoryTrib.__kmemoryStream.vaddrSpaceStream.releasePages(
		(void *)((uarch_t)ret & PAGING_BASE_MASK_HIGH), 1);

	ret = new ((memoryTrib.__kmemoryStream.vaddrSpaceStream
		.*memoryTrib.__kmemoryStream.vaddrSpaceStream.getPages)(
			cfgNPages))
		x86_mpCfgS;

	nMapped = walkerPageRanger::mapInc(
		&memoryTrib.__kmemoryStream.vaddrSpaceStream.vaddrSpace,
		ret, cfgPaddr, cfgNPages,
		PAGEATTRIB_PRESENT | PAGEATTRIB_SUPERVISOR);

	if (nMapped < static_cast<status_t>( cfgNPages ))
	{
		__kprintf(ERROR x86MP"Failed to map MP Config Table.\n");
		return __KNULL;
	};

	ret = (x86_mpCfgS *)(
		(uarch_t)ret + (cache.fp->cfgTablePaddr & PAGING_BASE_MASK_LOW));

	// Now we can return the table vaddr.
	return ret;
}

status_t x86Mp::getChipsetDefaultConfig(x86_mpFpS *mpfp)
{
	/* Determines whether or not the chipset uses an MP default config.
	 * Returns negative value if not, returns value greater than 0 if yes.
	 *
	 * The value returned, if greater than 0, is an index into a hardcoded
	 * table of CPU Config and CPU Maps to return for each default config.
	 **/
	if (mpfp->features[0] == 0) {
		return -1;
	};
	return mpfp->features[0];
}

void x86Mp::buildCacheData(x86_mpFpS *mpfp, x86_mpCfgS *cfg)
{
	cache.fp = mpfp;
	cache.cfg = cfg;

	if (cfg != __KNULL)
	{
		cache.lapicPaddr = cfg->lapicPaddr;
		if (__KFLAG_TEST(
			mpfp->features[0], x86_MPFP_FEAT2_FLAG_PICMODE))
		{
			__KFLAG_SET(cache.flags, x86_MPCACHE_FLAGS_WAS_PICMODE);
		};

		memcpy(cache.oemId, cfg->oemIdString, 8);
		cache.oemId[8] = '\0';
		memcpy(cache.oemProductId, cfg->oemProductIdString, 12);
		cache.oemProductId[12] = '\0';
	}
	else
	{
		cache.lapicPaddr = 0xFEC00000;
		cache.oemId[0] = '\0';
		cache.oemProductId[0] = '\0';
	};
}

