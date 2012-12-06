#ifndef _DEBUG_MODULE_H
	#define _DEBUG_MODULE_H

	#include <chipset/zkcm/device.h>
	#include <__kstdlib/__ktypes.h>

struct zkcmDebugDeviceC
:
public zkcmDeviceBaseC
{
public:
	enum deviceTypeE
	{
		TERMINAL=0, SERIAL, NETWORK, ROM, RAMBUFFER, OTHER
	} deviceType;

	zkcmDebugDeviceC(
		deviceTypeE devType,
		zkcmDeviceC *device
		)
	:
	zkcmDeviceBaseC(device),
	deviceType(devType)
	{}

public:
	// Basic control.
	virtual error_t	initialize(void);
	virtual error_t	shutdown(void);
	virtual error_t	suspend(void);
	virtual error_t	restore(void);

	// Interface to take input from debugPipeC::printf().
	virtual sarch_t isInitialized(void);
	virtual void syphon(const utf8Char *str, uarch_t len);
	virtual void clear(void);

public:
	// deviceType=SERIAL || NETWORK.
	struct
	{
		ubit32		dataRate;
		// deviceType=ROM || RAMBUFFER || NETWORK
		uarch_t		bufferSize;
		// deviceType=TERMINAL
		ubit16		lineWidth, nLines;
	} deviceDetails;
};

#endif

