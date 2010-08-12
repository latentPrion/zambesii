#ifndef _FIRMWARE_TRIB_H
	#define _FIRMWARE_TRIB_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/firmwareTrib/firmwareStream.h>

class firmwareTribC
:
public tributaryC
{
public:
	firmwareTribC(void);
	error_t initialize(void);
	~firmwareTribC(void);

public:
	firmwareStreamS *getChipsetStream(void);
	firmwareStreamS *getFirmwareStream(void);

	watchdogRivS *getWatchdogRiv(void);
	debugRivS *getDebugRiv1(void);
	debugRivS *getDebugRiv2(void);
	debugRivS *getDebugRiv3(void);
	debugRivS *getDebugRiv4(void);

public:
	firmwareStreamS		descriptor;
};

extern firmwareTribC		firmwareTrib;

#endif
