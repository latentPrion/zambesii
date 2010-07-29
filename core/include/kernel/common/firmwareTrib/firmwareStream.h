#ifndef _FIRMWARE_STREAM_H
	#define _FIRMWARE_STREAM_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/firmwareTrib/debugSupportRiv.h>

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
	struct debugSupportRivS		*debugSupportRiv1;
	struct debugSupportRivS		*debugSupportRiv2;
	struct debugSupportRivS		*debugSupportRiv3;
	struct debugSupportRivS		*debugSupportRiv4;
};

extern struct firmwareStreamS		firmwareFwStream;
extern struct firmwareStreamS		chipsetFwStream;

#endif

