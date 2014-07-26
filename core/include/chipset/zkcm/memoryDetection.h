#ifndef _ZKCM_MEMORY_DETECTION_H
	#define _ZKCM_MEMORY_DETECTION_H

	#include <chipset/zkcm/memoryConfig.h>
	#include <chipset/zkcm/numaMap.h>
	#include <chipset/zkcm/memoryMap.h>
	#include <__kstdlib/__ktypes.h>

class ZkcmMemoryDetectionMod
{
public:
	error_t initialize(void);
	error_t shutdown(void);
	error_t suspend(void);
	error_t restore(void);

	zkcmMemConfigS *getMemoryConfig(void);
	zkcmNumaMapS *getNumaMap(void);
	zkcmMemMapS *getMemoryMap(void);

	// Memory hotplug API design proposed for a later date.
};

#endif

