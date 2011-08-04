
#include <arch/paddr_t.h>
#include <arch/walkerPageRanger.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kclasses/debugPipe.h>
#include <commonlibs/libx86mp/lapic.h>
#include <commonlibs/libx86mp/mpTables.h>
#include <kernel/common/memoryTrib/memoryTrib.h>


#define x86_LAPIC_NPAGES		4

static struct x86LapicCacheS	cache;

void x86Lapic::initializeCache(void)
{
	if (cache.magic != x86LAPIC_MAGIC)
	{
		x86Lapic::flushCache();
		cache.magic = x86LAPIC_MAGIC;
	};
}

void x86Lapic::flushCache(void)
{
	memset8(&cache, 0, sizeof(cache));
}

void x86Lapic::setPaddr(paddr_t p)
{
	cache.p = p;
}

sarch_t x86Lapic::getPaddr(paddr_t *p)
{
	*p = cache.p;
	return (cache.p != 0);
}

error_t x86Lapic::mapLapicMem(void)
{
	paddr_t		p = cache.p;
	void		*v;
	status_t	nMapped;

	v = (memoryTrib.__kmemoryStream.vaddrSpaceStream
		.*memoryTrib.__kmemoryStream.vaddrSpaceStream.getPages)(
		x86_LAPIC_NPAGES);

	if (v == __KNULL)
	{
		__kprintf(ERROR x86LAPIC"Failed to get pages to map LAPIC.\n");
		return ERROR_MEMORY_NOMEM_VIRTUAL;
	};

	nMapped = walkerPageRanger::mapInc(
		&memoryTrib.__kmemoryStream.vaddrSpaceStream.vaddrSpace,
		v, p, x86_LAPIC_NPAGES,
		PAGEATTRIB_PRESENT | PAGEATTRIB_WRITE | PAGEATTRIB_SUPERVISOR
		| PAGEATTRIB_CACHE_WRITE_THROUGH);

	if (nMapped < x86_LAPIC_NPAGES)
	{
		__kprintf(ERROR x86LAPIC"Failed to map LAPIC at p 0x%X.\n", p);
		return ERROR_MEMORY_VIRTUAL_PAGEMAP;
	};

	cache.v = static_cast<ubit8 *>( v );
	return ERROR_SUCCESS;
}

ubit8 x86Lapic::read8(ubit32 offset)
{
	return *reinterpret_cast<ubit8 *>( (uarch_t)cache.v + offset );
}

ubit16 x86Lapic::read16(ubit32 offset)
{
	return *reinterpret_cast<ubit16 *>( (uarch_t)cache.v + offset );
}

ubit32 x86Lapic::read32(ubit32 offset)
{
	return *reinterpret_cast<ubit32 *>( (uarch_t)cache.v + offset );
}


void x86Lapic::write8(ubit32 offset, ubit8 val)
{
	*reinterpret_cast<ubit8 *>( (uarch_t)cache.v + offset ) = val;
}

void x86Lapic::write16(ubit32 offset, ubit16 val)
{
	*reinterpret_cast<ubit16 *>( (uarch_t)cache.v + offset ) = val;
}

void x86Lapic::write32(ubit32 offset, ubit32 val)
{
	*reinterpret_cast<ubit32 *>( (uarch_t)cache.v + offset ) = val;
}

