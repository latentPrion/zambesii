#ifndef _CHIPSET_MEMORY_CONFIG_H
	#define _CHIPSET_MEMORY_CONFIG_H

	#include <arch/paddr_t.h>

/**	EXPLANATION:
 * Interface used to allow a chipset porter to define the absolute size of RAM
 * at compile time.
 **/

struct chipsetMemConfigS
{
	paddr_t		memSize;
};

#endif

