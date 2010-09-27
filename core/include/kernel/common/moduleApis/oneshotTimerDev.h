#ifndef _ONE_SHOT_TIMER_DEVICE_API_H
	#define _ONE_SHOT_TIMER_DEVICE_API_H

	#include <__kstdlib/__ktypes.h>

struct oneshotTimerDevS
{
	error_t (*initialize)(void);
	error_t (*shutdown)(void);
	error_t (*suspend)(void);
	error_t (*awake)(void);

	void setTimeoutMs(uarch_t ms);
//	void setTimeoutNs(uarch_t ns);
};

#endif

