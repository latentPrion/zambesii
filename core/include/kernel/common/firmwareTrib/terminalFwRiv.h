#ifndef _TERMINAL_FIRMWARE_RIVULET_H
	#define _TERMINAL_FIRMWARE_RIVULET_H

	#include <__kstdlib/__ktypes.h>

struct terminalFwRivS
{
	// Basic control.
	error_t	(*initialize)(void);
	error_t	(*shutdown)(void);
	error_t	(*suspend)(void);
	error_t	(*awake)(void);

	// Interface to take input from debugPipeC::printf().
	void	(*read)(const unicodePoint *str);
	void	(*clear)(void);
};

#endif

