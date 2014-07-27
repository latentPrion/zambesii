#ifndef _CHIPSET_TERMINAL_FIRMWARE_RIVULET_H
	#define _CHIPSET_TERMINAL_FIRMWARE_RIVULET_H

	#include <__kstdlib/__ktypes.h>

struct sIbmPc_TerminalBuff
{
	ubit8	ch;
	ubit8	attr;
};

struct sTerminalFwRiv
{
	sIbmPc_TerminalBuff	*buffPtr;
};

#endif

