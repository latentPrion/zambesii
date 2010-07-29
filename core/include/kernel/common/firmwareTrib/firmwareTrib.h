#ifndef _FIRMWARE_TRIB_H
	#define _FIRMWARE_TRIB_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/firmwareTrib/watchdogFwRiv.h>
	#include <kernel/common/firmwareTrib/debugSupportRiv.h>

class firmwareTribC
:
public tributaryC
{
public:
	firmwareTribC(void);
	error_t initialize(void);
	~firmwareTribC(void);

public:
	watchdogFwRivS *getWatchdogFwRiv(void);
	debugSupportRivS *getDebugSupportRiv1(void);
	debugSupportRivS *getDebugSupportRiv2(void);
	debugSupportRivS *getDebugSupportRiv3(void);
	debugSupportRivS *getDebugSupportRiv4(void);

public:
	struct firmwareStateDescriptorS
	{
		watchdogFwRivS		*watchdogFwRiv;
		debugSupportRivS		*debugSupportRiv1;
		debugSupportRivS		*debugSupportRiv2;
		debugSupportRivS		*debugSupportRiv3;
		debugSupportRivS		*debugSupportRiv4;
	} descriptor;
};

extern firmwareTribC		firmwareTrib;

#endif
