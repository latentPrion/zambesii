#ifndef _WATCHDOG_CHIPSET_MODULE_H
	#define _WATCHDOG_CHIPSET_MODULE_H

	#include <__kstdlib/__ktypes.h>

class watchdogModS
{
public:
	error_t initialize(void);
	error_t shutdown(void);
	error_t suspend(void);
	error_t restore(void);

	status_t setAction(ubit8 action);
	status_t setTimeoutMs(ubit32 ms);
	status_t enable(void);
	status_t disable(void);
};

#endif

