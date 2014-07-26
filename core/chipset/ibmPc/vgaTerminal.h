#ifndef _IBM_PC_TERMINAL_MOD_H
	#define _IBM_PC_TERMINAL_MOD_H

	#include <chipset/zkcm/debugDevice.h>

class IbmPcVgaTerminal
:
public zkcmDebugDeviceC
{
public:
	IbmPcVgaTerminal(deviceTypeE devType)
	:
	zkcmDebugDeviceC(devType, &baseDeviceInfo),
	buff(NULL),
	baseDeviceInfo(
		0,
		CC"ibmpc-vga-term", CC"IBM PC compatible VGA card in text mode",
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
	void scrollDown(void);

private:
	struct ibmPc_terminalMod_fbS
	{
		ubit8	ch;
		ubit8	attr;
	};

	struct ibmPc_terminalMod_fbS	*buff, *origBuff;
	uarch_t				row, col, maxRow, maxCol;
	static ubit8			*bda;

	ZkcmDevice			baseDeviceInfo;
};

extern IbmPcVgaTerminal		ibmPcVgaTerminal;

#endif

