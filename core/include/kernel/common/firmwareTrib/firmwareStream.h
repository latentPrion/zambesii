#ifndef _FIRMWARE_STREAM_H
	#define _FIRMWARE_STREAM_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/stream.h>
	#include <kernel/common/firmwareTrib/terminalFwRiv.h>

/**	EXPLANATION:
 * The Firmware stream is a presentation layer for all the rivulets on either
 * a chipset or a firmware interface. This class holds instances of all the
 * rivulets on the stream.
 *
 * The Firmware Tributary will decide, upon being initialize()d, which rivulets
 * to stash away for kernel use.
 **/

struct firmwareStreamS
{
	terminalFwRivS		*terminalFwRiv;
};

extern firmwareStreamS		firmwareFwStream;
extern firmwareStreamS		chipsetFwStream;

#endif

