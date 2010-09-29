#ifndef _CPU_TIMER_H
	#define _CPU_TIMER_H

	#include <__kstdlib/__ktypes.h>

class schedTimerC
{
public:
	error_t initialize(void);
	void setHz(uarch_t hz);

public:
	uarch_t			hz;
	continuousTimerDevS	*timer;
};

#endif

