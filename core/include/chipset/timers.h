#ifndef _CHIPSET_TIMERS_H
	#define _CHIPSET_TIMERS_H

	#include <__kstdlib/__ktypes.h>

	/**	EXPLANATION:
	 * The kernel uses this bit array to determine which timer period
	 * queues it should spawn for the chipset it is running on. The
	 * chipset defines this bitmap value in its chipset specific code.
	 *
	 * Not every chipset can operate stably with IRQs coming in every 1ms,
	 * for example. The chipset defines the bits which represent the timer
	 * periods which are /safe/ for models of that chipset. Generally though
	 * if a chipset model has a timer of a certain granularity, it was
	 * placed there by design, and the chipset was meant to process
	 * data that required timing at that frequency, so most chipsets will
	 * end up just defining all the frequencies supported by the timers on
	 * their model as safe.
	 **/

#define CHIPSET_TIMERS_1S_SAFE		(1<<0)
#define CHIPSET_TIMERS_100MS_SAFE	(1<<1)
#define CHIPSET_TIMERS_10MS_SAFE	(1<<2)
#define CHIPSET_TIMERS_1MS_SAFE		(1<<3)
#define CHIPSET_TIMERS_100NS_SAFE	(1<<4)
#define CHIPSET_TIMERS_10NS_SAFE	(1<<5)
#define CHIPSET_TIMERS_1NS_SAFE		(1<<6)

extern ubit32		chipsetSafeTimerQueues;

#endif

