#ifndef _CHIPSET_TERMINAL_FIRMWARE_RIVULET_H
	#define _CHIPSET_TERMINAL_FIRMWARE_RIVULET_H

	#include <__kstdlib/__ktypes.h>

struct ibmPc_terminalBuffS
{
	ubit8	ch;
	ubit8	attr;
};

struct terminalFwRivS
{
	ibmPc_terminalBuffS	*buffPtr;
};

#endif

