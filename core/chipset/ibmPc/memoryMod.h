#ifndef _IBM_PC_MEMORY_MOD_H
	#define _IBM_PC_MEMORY_MOD_H

	#include <chipset/pkg/memoryMod.h>

#ifdef __cplusplus
#define IPMMEXTERN		extern "C"
#else
#define IPMMEXTERN		extern
#endif

IPMMEXTERN error_t ibmPc_memoryMod_initialize(void);
IPMMEXTERN error_t ibmPc_memoryMod_shutdown(void);
IPMMEXTERN error_t ibmPc_memoryMod_suspend(void);
IPMMEXTERN error_t ibmPc_memoryMod_restore(void);

IPMMEXTERN struct chipsetNumaMapS *ibmPc_memoryMod_getNumaMap(void);
IPMMEXTERN struct chipsetMemMapS *ibmPc_memoryMod_getMemoryMap(void);
IPMMEXTERN struct chipsetMemConfigS *ibmPc_memoryMod_getMemoryConfig(void);

IPMMEXTERN struct memoryModS	ibmPc_memoryMod;

#endif

