#ifndef _IBM_PC_BIOS_CORE_FUNCTIONS_H
	#define _IBM_PC_BIOS_CORE_FUNCTIONS_H

	#include <__kstdlib/__ktypes.h>
	#include "ibmPcBios_regManip.h"

#ifdef __cplusplus
extern "C" {
#endif

void ibmPcBios_lock_acquire(void);
void ibmPcBios_lock_release(void);
void ibmPcBios_executeInterrupt(ubit8 intNo);

#ifdef __cplusplus
}
#endif

#endif

