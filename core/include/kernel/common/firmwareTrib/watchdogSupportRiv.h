#ifndef _WATCHDOG_SUPPORT_RIVULET_H
	#define _WATCHDOG_SUPPORT_RIVULET_H

/**	EXPLANATION:
 * Watchdog Firmware Support Rivulet API.
 *
 * The watchdog device must provide an ISR for the kernel to call, and that ISR
 * must feed the watchdog, and then return. Other entry points into the driver,
 * and what they must do are:
 *	enable():	Enable the watchdog if it's not enabled already.
 *	disable():	disable the watchdog it it's running.
 *	suspend():	halt the watchdog's countdown until the kernel calls
 *		restore().
 *	restore():	cause the watchdog to resume counting down. It is
 *		preferable if the count is resumed from a "full" load.
 *	setAction(): 	Set the watchdog to do one of the following:
 *		SHUTDOWN, RESET, DO_NOTHING. Default behaviour of a watchdog is
 *		assumed to be RESET, although a watchdog which does not do this
 *		is not a problem.
 *		If your device gets this call with an action you do not support
 *		then you should return any value other than ERROR_SUCCESS.
 *	setInterval(): 	Set the interval for your watchdog device to take as
 *		the time during which the system is still operational.
 *		The kernel passes an unsigned number in milliseconds.
 **/

#define WATCHDOG_ACTION_RESET		0x0
#define WATCHDOG_ACTION_SHUTDOWN	0x1
#define WATCHDOG_ACTION_DO_NOTHING	0x2

struct watchdogSupportRivS
{
	// ISR to feed the watchdog.
	status_t (*isr)();

	error_t (*enable)();
	error_t (*disable)();
	error_t (*suspend)();
	error_t (*restore)();
	sarch_t (*isEnabled);

	// Main API:
	error_t (*setInterval)(uarch_t interval);
	void (*setAction)(uarch_t action);
};

#endif

