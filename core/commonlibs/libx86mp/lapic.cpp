
#include <arch/paddr_t.h>
#include <commonlibs/libx86mp/lapic.h>
#include <commonlibs/libx86mp/mpTables.h>
#include <arch/walkerPageRanger.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/memoryTrib/memoryTrib.h>


#define x86_LAPIC_NPAGES		4

static struct x86LapicCacheS	cache;

error_t x86Lapic::mapLapicMem(void)
{
	paddr_t		p = x86Mp::getLapicPaddr();
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

	cache.v = static_cast<ubit32 *>( v );
	return ERROR_SUCCESS;
}

