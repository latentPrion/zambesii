#ifndef _IBM_PC_BIOS_CORE_FUNCTIONS_H
	#define _IBM_PC_BIOS_CORE_FUNCTIONS_H

	#include <__kstdlib/__ktypes.h>
	#include "ibmPcBios_regManip.h"

namespace ibmPcBios
{
	error_t initialize(void);
	error_t shutdown(void);
	error_t suspend(void);
	error_t restore(void);

	void acquireLock(void);
	void releaseLock(void);
	void executeInterrupt(ubit8 intNo);
}

#endif

