
#include <arch/paddr_t.h>
#include <arch/paging.h>
#include <arch/walkerPageRanger.h>
#include <chipset/memoryAreas.h>
#include <kernel/common/memoryTrib/memoryTrib.h>


chipsetMemAreas::areaS		memAreas[CHIPSET_MEMAREA_NAREAS] =
{
	{
		CC"Low Memory (0x0-0xFFFFF, 1MB)",
		0x0, 0x100000,
		__KNULL
	}
};

/**	EXPLANATION:
 **/

error_t chipsetMemAreas::mapArea(ubit16 index)
{
	void		*vaddr;
	status_t	status;

	if (index >= CHIPSET_MEMAREA_NAREAS) { return ERROR_INVALID_ARG_VAL; };
	if (memAreas[index].vaddr != __KNULL) { return ERROR_SUCCESS; };

	vaddr = (memoryTrib.__kmemoryStream.vaddrSpaceStream
		.*memoryTrib.__kmemoryStream.vaddrSpaceStream.getPages)(
			PAGING_BYTES_TO_PAGES(memAreas[index].size));

	if (vaddr == __KNULL) { return ERROR_MEMORY_NOMEM_VIRTUAL; };

	// Have vmem to play with. Map it to the memory area requested.
	status = walkerPageRanger::mapInc(
		&memoryTrib.__kmemoryStream.vaddrSpaceStream.vaddrSpace,
		vaddr,
		memAreas[index].basePaddr,
		PAGING_BYTES_TO_PAGES(memAreas[index].size),
		PAGEATTRIB_PRESENT | PAGEATTRIB_WRITE | PAGEATTRIB_SUPERVISOR
		| PAGEATTRIB_CACHE_WRITE_THROUGH);

	if (status < PAGING_BYTES_TO_PAGES(memAreas[index].size))
	{
		memoryTrib.__kmemoryStream.vaddrSpaceStream.releasePages(
			vaddr, PAGING_BYTES_TO_PAGES(memAreas[index].size));

		return ERROR_MEMORY_VIRTUAL_PAGEMAP;
	};

	// We have now mapped the area in.
	vaddr = WPRANGER_ADJUST_VADDR(vaddr, memAreas[index].basePaddr, void *);
	memAreas[index].vaddr = vaddr;
	return ERROR_SUCCESS;
}

error_t chipsetMemAreas::unmapArea(ubit16 index)
{
	paddr_t		p;
	uarch_t		f;

	if (index >= CHIPSET_MEMAREA_NAREAS) { return ERROR_INVALID_ARG_VAL; };
	if (memAreas[index].vaddr == __KNULL) { return ERROR_SUCCESS; };

	walkerPageRanger::unmap(
		&memoryTrib.__kmemoryStream.vaddrSpaceStream.vaddrSpace,
		memAreas[index].vaddr,
		&p,
		PAGING_BYTES_TO_PAGES(memAreas[index].size),
		&f);

	memoryTrib.__kmemoryStream.vaddrSpaceStream.releasePages(
		memAreas[index].vaddr,
		PAGING_BYTES_TO_PAGES(memAreas[index].size));

	memAreas[index].vaddr = __KNULL;
	return ERROR_SUCCESS;
}
		
void *chipsetMemAreas::getArea(ubit16 index)
{
	if (index >= CHIPSET_MEMAREA_NAREAS) { return __KNULL; };
	return memAreas[index].vaddr;
}

chipsetMemAreas::areaS *chipsetMemAreas::getAreaInfo(ubit16 index)
{
	if (index >= CHIPSET_MEMAREA_NAREAS) { return __KNULL; };
	return &memAreas[index];
}

