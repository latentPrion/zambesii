#ifndef _TERMINAL_FIRMWARE_RIVULET_H
	#define _TERMINAL_FIRMWARE_RIVULET_H

#ifdef __cplusplus
	#include <__kstdlib/__ktypes.h>
#else
	#include <kernel/common/firmwareTrib/firmwareRivTypes.h>
#endif

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

