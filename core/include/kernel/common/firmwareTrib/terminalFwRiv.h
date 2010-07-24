#ifndef _TERMINAL_FIRMWARE_RIVULET_H
	#define _TERMINAL_FIRMWARE_RIVULET_H

	#include <chipset/terminalFwRiv.h>
	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/firmwareTrib/firmwareRiv.h>

#define DEBUGRIV_DEV_VIDEO	(1<<0)
#define DEBUGRIV_DEV_SERIAL	(1<<1)
#define DEBUGRIV_DEV_PARALLEL	(1<<2)
#define DEBUGRIV_DEV_NIC	(1<<3)

class terminalFwRivC
:
public firmwareRivC
{
public:
	terminalFwRivC(void) {};
	~terminalFwRivC(void) {};

public:
	virtual error_t initialize(void);
	virtual error_t suspend(void);
	virtual error_t awake(void);
	virtual error_t shutdown(void);

public:
	void write(utf16Char *buff);
	void clear(void);

private:
	terminalFwRivS		riv;
};

#endif

