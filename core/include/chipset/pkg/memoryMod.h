#ifndef _CHIPSET_MEMORY_INFO_MODULE_H
	#define _CHIPSET_MEMORY_INFO_MODULE_H

	#include <chipset/memoryConfig.h>
	#include <chipset/memoryMap.h>
	#include <chipset/numaMap.h>
	#include <__kstdlib/__ktypes.h>

struct memoryModS
{
	error_t (*initialize)(void);
	error_t (*shutdown)(void);
	error_t (*suspend)(void);
	error_t (*restore)(void);

	// Memory information querying interface.
	struct chipsetNumaMapS *(*getNumaMap)(void);
	struct chipsetMemMapS *(*getMemoryMap)(void);
	struct chipsetMemConfigS *(*getMemoryConfig)(void);
};

#endif

