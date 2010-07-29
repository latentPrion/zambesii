#ifndef _FIRMWARE_STREAM_H
	#define _FIRMWARE_STREAM_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/firmwareTrib/watchdogSupportRiv.h>
	#include <kernel/common/firmwareTrib/debugSupportRiv.h>

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
 *	* !Watchdog management.
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
	struct watchdogSupportRivS	*watchdogSupportRiv;
	struct debugSupportRivS		*debugSupportRiv1;
	struct debugSupportRivS		*debugSupportRiv2;
	struct debugSupportRivS		*debugSupportRiv3;
	struct debugSupportRivS		*debugSupportRiv4;
};

extern struct firmwareStreamS		firmwareFwStream;
extern struct firmwareStreamS		chipsetFwStream;

#endif

