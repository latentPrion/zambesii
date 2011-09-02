#ifndef _IBM_PC_CPU_INFO_MODULE_H
	#define _IBM_PC_CPU_INFO_MODULE_H

	#include <chipset/pkg/chipsetPackage.h>

#ifdef __cplusplus
	#define IPCMEXTERN extern "C"
#else
	#define IPCMEXTERN extern
#endif

IPCMEXTERN error_t ibmPc_cpuMod_initialize(void);
IPCMEXTERN error_t ibmPc_cpuMod_shutdown(void);
IPCMEXTERN error_t ibmPc_cpuMod_suspend(void);
IPCMEXTERN error_t ibmPc_cpuMod_restore(void);

IPCMEXTERN struct chipsetNumaMapS *ibmPc_cpuMod_getNumaMap(void);
IPCMEXTERN struct chipsetSmpMapS *ibmPc_cpuMod_getSmpMap(void);
IPCMEXTERN cpu_t ibmPc_cpuMod_getBspId(void);
IPCMEXTERN sarch_t ibmPc_cpuMod_checkSmpSanity(void);
IPCMEXTERN status_t ibmPc_cpuMod_powerControl(
	cpu_t cpuId, ubit8 command, uarch_t flags);

IPCMEXTERN struct cpuModS ibmPc_cpuMod;

#endif

