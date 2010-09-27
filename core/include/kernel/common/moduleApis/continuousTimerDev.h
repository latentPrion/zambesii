#ifndef _CONTINUOUS_TIMER_API_H
	#define _CONTINUOUS_TIMER_API_H

	#include <__kstdlib/__ktypes.h>

struct continuousTimerDevS
{
	error_t (*initialize)(void);
	error_t (*shutdown)(void);
	error_t (*suspend)(void);
	error_t (*awake)(void);

	// Calling this with 0 disables the timer device.
	void (*setHz)(uarch_t hz);
};

#endif

