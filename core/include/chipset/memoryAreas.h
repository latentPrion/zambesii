#ifndef _CHIPSET_MEMORY_AREAS_H
	#define _CHIPSET_MEMORY_AREAS_H


	#include <arch/paddr_t.h>
	#include <__kstdlib/__ktypes.h>

	#include <chipset/chipset_include.h>
	#include CHIPSET_INCLUDE(memoryAreas.h)


namespace chipsetMemAreas
{
	struct areaS
	{
		utf8Char	*name;
		paddr_t		basePaddr;
		paddr_t		size;
		void		*vaddr;
	};

	error_t mapArea(ubit16 index);
	error_t unmapArea(ubit16 index);
	void *getArea(ubit16 index);
	struct areaS *getAreaInfo(ubit16 index);
}

#endif

