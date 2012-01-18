#ifndef _IBM_PC_MEMORY_MOD_H
	#define _IBM_PC_MEMORY_MOD_H

	#include <chipset/zkcm/memoryDetection.h>

#ifdef __cplusplus
#define IPMMEXTERN		extern "C"
#else
#define IPMMEXTERN		extern
#endif

IPMMEXTERN error_t ibmPc_memoryMod_initialize(void);
IPMMEXTERN error_t ibmPc_memoryMod_shutdown(void);
IPMMEXTERN error_t ibmPc_memoryMod_suspend(void);
IPMMEXTERN error_t ibmPc_memoryMod_restore(void);

IPMMEXTERN struct zkcmNumaMapS *ibmPc_memoryMod_getNumaMap(void);
IPMMEXTERN struct zkcmMemMapS *ibmPc_memoryMod_getMemoryMap(void);
IPMMEXTERN struct zkcmMemConfigS *ibmPc_memoryMod_getMemoryConfig(void);

IPMMEXTERN struct zkcmMemoryDetectionModS	ibmPc_memoryDetectionMod;

#endif

