#ifndef _FIRMWARE_STREAM_H
	#define _FIRMWARE_STREAM_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/firmwareTrib/watchdogRiv.h>
	#include <kernel/common/firmwareTrib/debugRiv.h>

/**	EXPLANATION:
 * The Firmware stream is a presentation layer for all the rivulets on either
 * a chipset or a firmware interface. This class holds instances of all the
 * rivulets on the stream.
 *
 * From a less "Zambezii" perspective, then, this is a descriptor structure
 * that states what services the firmware on the chipset is able to provide,
 * and what drivers the chipset provides in place of the firmware.
 *
 * Eventually, the services supported by the kernel through this interface
 * will be:
 *	* Watchdog management.
 *	* Up to four debug devices or services.
 *	* !Memory and !NUMA detection.
 *	* !Power management.
 *	* !Firmware-hosted device tree enumeration.
 *
 * (A '!' preceding the service indicates that support for it does not 
 * currently exist.)
 *
 * The Firmware Tributary will decide, upon being initialize()d, which rivulets
 * to stash away for kernel use.
 **/

struct firmwareStreamS
{
	error_t (*initialize)(void);
	error_t (*shutdown)(void);
	error_t (*suspend)(void);
	error_t (*awake)(void);

	// Kernel interrupt vector control.
	/* initializeInterrupts() expects the chipset to place itself in a state
	 * that will allow the kernel to request masking and unmasking of
	 * IRQs.
	 *
	 * Additionally, the chipset must see to any watchdog timer that it
	 * has on board. During this call, the chipset is expected to, if it has
	 * a watchdog, feed it, and register an ISR with the timer tributary to
	 * enable the kernel to continue to feed it.
	 *
	 * This implies that if a chipset has a watchdog, it must initialize a
	 * continuous timer device during initializeInterrupts(). As a result,
	 * the chipset is also expected to route that timer IRQ to the
	 * bootstrap processor, and enable IRQs on the BSP as well.
	 *
	 * Note well that Zambezii will *never* statically enable or disable
	 * IRQs on the BSP until after initializeInterrupts2() has been called.
	 * The chipset is expected to know WHEN and HOW to
	 * permanently enable IRQs on the BSP. This can be done at one of two
	 * times: When initializeInterrupts() is called, the chipset can do
	 * a local IRQ enable on the BSP, or later on when the kernel calls
	 * initializeInterrupts2(). If not done up to then, the kernel will just
	 * enable local IRQs on the BSP and continue.
	 *
	 * initializeInterrupts() is called AS SOON AS POSSIBLE after the
	 * kernel is jumped to to ensure that any watchdog device is fed. At
	 * that point, there is no access to memory management.
	 *
	 * initalizeInterrupts2() is called later on when the kernel is ready to
	 * set up software ISRs. At this point, the kernel has Memory allocation
	 * and an operational heap.
	 **/
	error_t	(*initializeInterrupts)(void);
	error_t (*initializeInterrupts2)(void);
	error_t (*maskSingle)(uarch_t vector);
	error_t (*maskAll)(void);
	error_t (*unmaskSingle)(uarch_t vector);
	error_t (*unmaskAll)(void);

	struct watchdogRivS	*watchdogRiv;
	struct debugRivS	*debugRiv1;
	struct debugRivS	*debugRiv2;
	struct debugRivS	*debugRiv3;
	struct debugRivS	*debugRiv4;
};

extern struct firmwareStreamS		firmwareFwStream;
extern struct firmwareStreamS		chipsetFwStream;

#endif

