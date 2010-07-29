#ifndef _TERMINAL_SUPPORT_RIVULET_H
	#define _TERMINAL_SUPPORT_RIVULET_H

	#include <__kstdlib/__ktypes.h>

struct debugSupportRivS
{
	// Basic control.
	error_t	(*initialize)(void);
	error_t	(*shutdown)(void);
	error_t	(*suspend)(void);
	error_t	(*restore)(void);
	sarch_t	(*isInitialized)(void);

	// Interface to take input from debugPipeC::printf().
	void	(*read)(const unicodePoint *str);
	void	(*clear)(void);
};

#endif

