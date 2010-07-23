#ifndef _FIRMWARE_TRIB_H
	#define _FIRMWARE_TRIB_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/tributary.h>

class firmwareTribC
:
public tributaryC
{
public:
	firmwareTribC(void);
	error_t initialize(void);
	~firmwareTribC(void);

public:
	debugRiv *getDebugRiv(void);

public:
	firmwareStateDescriptorS	stateDescriptor;
};

extern firmwareTribC		firmwareTrib;

#endif

firmwareTrib.getDebugRiv()->printf(
