#ifndef _DEBUG_MODULE_H
	#define _DEBUG_MODULE_H

	#include <__kstdlib/__ktypes.h>

struct debugModS
{
	// Basic control.
	error_t	(*initialize)(void);
	error_t	(*shutdown)(void);
	error_t	(*suspend)(void);
	error_t	(*restore)(void);

	// Interface to take input from debugPipeC::printf().
	sarch_t	(*isInitialized)(void);
	void	(*syphon)(const utf8Char *str, uarch_t len);
	void	(*clear)(void);
};

#endif

