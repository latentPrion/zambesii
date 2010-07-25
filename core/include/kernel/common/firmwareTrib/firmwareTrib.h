#ifndef _FIRMWARE_TRIB_H
	#define _FIRMWARE_TRIB_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/firmwareTrib/debugRiv.h>

class firmwareTribC
:
public tributaryC
{
public:
	firmwareTribC(void);
	error_t initialize(void);
	~firmwareTribC(void);

public:
	debugRivC *getDebugRiv(void);

public:
	struct firmwareStateDescriptorS
	{
		debugRivC	*debugRiv;
	} stateDescriptor;
};

extern firmwareTribC		firmwareTrib;

#endif
