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

class firmwareStreamC
:
public streamC
{
public:
	firmwareStreamC(void);
	error_t initialize(void);
	~firmwareStreamC(void);

public:
	terminalFwRivC		terminalFwRiv;
};

extern firmwareStreamC		firmwareFwStream;
extern firmwareStreamC		chipsetFwStream;

#endif

