
#include <arch/paddr_t.h>
#include <arch/paging.h>
#include <arch/walkerPageRanger.h>
#include <chipset/memoryAreas.h>
#include <kernel/common/processTrib/processTrib.h>


chipsetMemAreas::sArea		memAreas[CHIPSET_MEMAREA_NAREAS] =
{
	{
		CC"Low Memory (0x0-0xFFFFF, 1MB)",
		0x0, 0x100000,
		NULL
	}
};

/**	EXPLANATION:
 **/

error_t chipsetMemAreas::mapArea(ubit16 index)
{
	void		*vaddr;
	status_t	status;

	if (index >= CHIPSET_MEMAREA_NAREAS) { return ERROR_INVALID_ARG_VAL; };
	if (memAreas[index].vaddr != NULL) { return ERROR_SUCCESS; };

#ifdef CONFIG_DEBUGPIPE_STATIC
	if (index == CHIPSET_MEMAREA_LOWMEM) { vaddr = (void *)0xCF800000; };
	goto skipVmemAlloc;
#endif
	vaddr = processTrib.__kgetStream()->getVaddrSpaceStream()->getPages(
		PAGING_BYTES_TO_PAGES(memAreas[index].size).getLow());

#ifdef CONFIG_DEBUGPIPE_STATIC
skipVmemAlloc:
#endif
	if (vaddr == NULL) { return ERROR_MEMORY_NOMEM_VIRTUAL; };

	// Have vmem to play with. Map it to the memory area requested.
	status = walkerPageRanger::mapInc(
		&processTrib.__kgetStream()->getVaddrSpaceStream()->vaddrSpace,
		vaddr,
		memAreas[index].basePaddr,
		PAGING_BYTES_TO_PAGES(memAreas[index].size).getLow(),
		PAGEATTRIB_PRESENT | PAGEATTRIB_WRITE | PAGEATTRIB_SUPERVISOR
		| PAGEATTRIB_CACHE_WRITE_THROUGH);

	if (status < (signed)PAGING_BYTES_TO_PAGES(
		memAreas[index].size).getLow())
	{
		processTrib.__kgetStream()->getVaddrSpaceStream()->releasePages(
			vaddr,
			PAGING_BYTES_TO_PAGES(memAreas[index].size).getLow());

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
	if (memAreas[index].vaddr == NULL) { return ERROR_SUCCESS; };

	walkerPageRanger::unmap(
		&processTrib.__kgetStream()->getVaddrSpaceStream()->vaddrSpace,
		memAreas[index].vaddr,
		&p,
		PAGING_BYTES_TO_PAGES(memAreas[index].size).getLow(),
		&f);

	processTrib.__kgetStream()->getVaddrSpaceStream()->releasePages(
		memAreas[index].vaddr,
		PAGING_BYTES_TO_PAGES(memAreas[index].size).getLow());

	memAreas[index].vaddr = NULL;
	return ERROR_SUCCESS;
}

void *chipsetMemAreas::getArea(ubit16 index)
{
	if (index >= CHIPSET_MEMAREA_NAREAS) { return NULL; };
	return memAreas[index].vaddr;
}

chipsetMemAreas::sArea *chipsetMemAreas::getAreaInfo(ubit16 index)
{
	if (index >= CHIPSET_MEMAREA_NAREAS) { return NULL; };
	return &memAreas[index];
}

