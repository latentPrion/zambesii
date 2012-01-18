#ifndef _CHIPSET_MEMORY_CONFIG_H
	#define _CHIPSET_MEMORY_CONFIG_H

	#include <arch/paddr_t.h>

/**	EXPLANATION:
 * Interface used to allow a chipset porter to define the absolute size of RAM
 * at compile time.
 **/

struct zkcmMemConfigS
{
	// Just in case some chipset doesn't have memory starting at paddr 0x0.
	paddr_t		memBase;
	paddr_t		memSize;
};

#endif

