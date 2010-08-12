#ifndef _WATCHDOG_MODULE_API_H
	#define _WATCHDOG_MODULE_API_H

	#include <__kstdlib/__ktypes.h>

/**	EXPLANATION:
 * Watchdog Module API.
 *
 * The watchdog device must provide an ISR for the kernel to call, and that ISR
 * must feed the watchdog, and then return. Other entry points into the driver,
 * and what they must do are:
 *
 *	initialize():	Enable the watchdog if it's not enabled already.
 *	shutdown():	Disable the watchdog it it's running.
 *	suspend():	Halt the watchdog's countdown until the kernel calls
 *			awake().
 *	awake():	Cause the watchdog to resume counting down. It is
 *			preferable if the count is resumed from a "full" load.
 *			That is, it would be very nice of you if awake() would
 *			re-initialize the countdown to 0 so that the kernel has
 *	setAction(): 	Set the watchdog to do one of the following at timeout:
 *			SHUTDOWN, RESET, DO_NOTHING. Default behaviour of a
 *			watchdog is assumed to be RESET, although a watchdog
 *			which does not do this is not a problem. If your device
 *			gets this call with an action you do not support then
 *			you should return any value other than ERROR_SUCCESS.
 *	setInterval(): 	Set the interval for your watchdog device to take as
 *			the time during which the system is still operational.
 *			The kernel passes an unsigned number in milliseconds.
 *
 * Unlike most other devices, the kernel assumes that at boot, the watchdog
 * device is *already* initialize()d. That means that Zambezii will expect a
 * watchdog to be fully active if it detects the existence of one (detected by
 * means of a non-NULL pointer in the chipset support package).
 **/

#define WATCHDOG_ACTION_RESET		0x0
#define WATCHDOG_ACTION_SHUTDOWN	0x1
#define WATCHDOG_ACTION_DO_NOTHING	0x2

struct watchdogDevS
{
	/**	NOTE:
	 * ISR to feed the watchdog. The kernel expects a watchdog driver's ISR
	 * to feed its device, and exit immediately. There is no need whatsoever
	 * for a watchdog device to schedule an IST. Therefore, a watchdog
	 * driver's ISR takes no arguments: it needs no context.
	 *
	 * All it needs to do is feed the watchdog, and exit. Please make sure
	 * your driver's ISR conforms to this requirement.
	 **/
	status_t (*isr)();

	error_t (*initialize)();
	error_t (*shutdown)();
	error_t (*suspend)();
	error_t (*restore)();
	sarch_t (*isEnabled);

	// Main API:
	error_t (*setInterval)(uarch_t interval);
	void (*setAction)(uarch_t action);
};

#endif

