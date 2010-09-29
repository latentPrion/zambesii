#ifndef _CHIPSET_SUPPORT_PACKAGE_H
	#define _CHIPSET_SUPPORT_PACKAGE_H

	#include <kernel/common/moduleApis/watchdog.h>
	#include <kernel/common/moduleApis/interruptController.h>

/**	EXPLANATION:
 * There is to be only *one* chipset support package per chipset. This implies
 * a constraint of only one chipset support package per kernel build.
 *
 * There are several very crucial device drivers which cannot be excluded from
 * the kernel image, no matter how much you try. Zambezii will generally
 * postpone driver loading until much later on in kernel initialization, when
 * the VFS has been initialized and the EndoKernel FS is parsable.
 *
 * However, in order to provide support for watchdog feeding at boot time,
 * there is a need to link in a driver for watchdog feeding wherever a watchdog
 * exists.
 *
 * In order to support handling of unknown IRQs gracefully so that they do not
 * cause the kernel to have an IRQ latched and repeatedly coming in, there is a
 * need for an in-kernel static driver for interrupt controller communication.
 *
 * The Timer Tributary will scan the chipset's support package for the existence
 * of a watchdog device driver on boot. If the pointer is non-NULL, the kernel
 * will call initialize() on the presumed watchdog device. The device is
 * expected to initialize a chipset timer and calibrate it to 1000Hz, then
 * register the ISR for the watchdog's feeding with the Timer Tributary using
 * the relevant API call. The kernel will handle the calling of the watchdog
 * feeding ISR from there. If a watchdog's initialize() does not return
 * ERROR_SUCCESS, the kernel will panic and halt.
 *
 * The Interrupt Tributary will scan the chipset support package for the
 * existence of an interrupt controller. If the pointer is non-NULL, the kernel
 * will call initialize() on the detected interrupt controller. The existence
 * of an interrupt controller is *required* for every chipset. If the kernel
 * detects no interrupt controller driver in the chipset support package,
 * it will panic and halt. The kernel will also panic if initialize() does not
 * return ERROR_SUCCESS. Immediately after the kernel calls initialize() on
 * the interrupt controller driver, it will make a call to maskAll().
 *
 * The chipset support package is a bundle of crucial drivers which the kernel
 * cannot do without for a chipset. Every other driver is loaded from the EKFS.
 **/

struct chipsetSupportPackageS
{
	struct watchdogDevS		*(*getWatchdogDev)(
		struct watchdogDevS *w);

	struct intControllerDevS	*(*getIntController)(
		struct intControllerDevS *i);

	struct continuousTimerDevS	*(*getSchedTimer)(
		struct continuousTimerDevS *s);
};

extern struct chipsetSupportPackageS	chipsetCoreDev;

#endif

