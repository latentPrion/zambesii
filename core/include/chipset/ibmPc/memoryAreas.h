#ifndef _CHIPSET_IBM_PC_MEMORY_AREAS_H
	#define _CHIPSET_IBM_PC_MEMORY_AREAS_H

	#include <lang.h>
	#include <arch/paddr_t.h>
	#include <__kstdlib/__ktypes.h>

#define CHIPSET_MEMAREA_NAREAS		1

#define CHIPSET_MEMAREA_LOWMEM		0

struct chipsetMemAreaS
{
	utf8Char	*name;
	paddr_t		basePaddr;
	paddr_t		size;
	void		*vaddr;
};

EXTERN error_t chipset_mapArea(ubit16 index);
EXTERN error_t chipset_unmapArea(ubit16 index);
EXTERN void *chipset_getArea(ubit16 index);
EXTERN struct chipsetMemAreaS *chipset_getAreaInfo(ubit16 index);

#endif

