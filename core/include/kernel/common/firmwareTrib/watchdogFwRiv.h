#ifndef _WATCHDOG_FIRMWARE_RIVULET_H
	#define _WATCHDOG_FIRMWARE_RIVULET_H

/**	EXPLANATION:
 * Watchdog Rivulet API.
 * Completely incomplete...
 **/
struct watchdogFwRivS
{
	error_t (*initialize)();
	error_t (*shutdown)();
	error_t (*suspend)();
	error_t (*awake)();
};

#endif

