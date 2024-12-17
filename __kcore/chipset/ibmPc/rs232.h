#ifndef _IBM_PC_RS232_MOD_H
	#define _IBM_PC_RS232_MOD_H

	#include <chipset/zkcm/debugDevice.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

class IbmPcRs232
:
public ZkcmDebugDevice
{
public:
	IbmPcRs232(ubit32 childId)
	:
	ZkcmDebugDevice(ZkcmDebugDevice::TERMINAL, &baseDeviceInfo),
	baseDeviceInfo(
		childId,
		CC"ibmpc-rs232-serial", CC"IBM PC compatible RS232 serial",
		CC"Unknown vendor", CC"N/A")
	{}

public:
	virtual error_t	initialize(void);
	virtual error_t	shutdown(void);
	virtual error_t	suspend(void);
	virtual error_t	restore(void);

	// Interface to take input from DebugPipe::printf().
	virtual sarch_t isInitialized(void);
	virtual void syphon(const utf8Char *str, uarch_t len);
	virtual void clear(void);

	void chipsetEventNotification(ubit8 event, uarch_t flags);

private:
	// Serial is a character stream, there's no surface to scroll
	void scrollDown(void) {}

private:
	struct sState
	{
		sState(void)
		:
		port(0)
		{}

		ubit16		port;
	}s;

	ZkcmDevice			baseDeviceInfo;
};

extern IbmPcRs232		ibmPcRs2320, ibmPcRs2321;

#endif
