#ifndef _FIRMWARE_TRIB_H
	#define _FIRMWARE_TRIB_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/firmwareTrib/terminalFwRiv.h>

class firmwareTribC
:
public tributaryC
{
public:
	firmwareTribC(void);
	error_t initialize(void);
	~firmwareTribC(void);

public:
	terminalFwRivS *getTerminalFwRiv(void);

public:
	struct firmwareStateDescriptorS
	{
		terminalFwRivS		*terminalFwRiv;
	} descriptor;
};

extern firmwareTribC		firmwareTrib;

#endif
