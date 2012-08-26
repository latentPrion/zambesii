#ifndef _ZKCM_TIMER_CONTROL_MODULE_H
	#define _ZKCM_TIMER_CONTROL_MODULE_H

	#include <__kstdlib/__ktypes.h>

	/**	EXPLANATION:
	 * The Timer Control module, part of the Zambesii Kernel Chipset
	 * Module API aggregate, is the interface that the kernel calls on to
	 * manage timer sources on a chipset. As a rule, the kernel will
	 * directly manage timer devices within ZKCM. However, there is no
	 * plan to disallow UDI drivers for timers on a chipset.
	 *
	 * The Timer Control mod will present the kernel with a list of timer
	 * sources and, if the kernel ever becomes interested, timer devices,
	 * which can be filtered based on programming I/O latency, supported
	 * frequencies, and any other criteria necessary. A timer device may
	 * provide more than one timer source.
	 **/

// Safe timer period bitmap: bit position significance.
#define CHIPSET_TIMERS_1S_SAFE		(1<<0)
#define CHIPSET_TIMERS_100MS_SAFE	(1<<1)
#define CHIPSET_TIMERS_10MS_SAFE	(1<<2)
#define CHIPSET_TIMERS_1MS_SAFE		(1<<3)
#define CHIPSET_TIMERS_100NS_SAFE	(1<<4)
#define CHIPSET_TIMERS_10NS_SAFE	(1<<5)
#define CHIPSET_TIMERS_1NS_SAFE		(1<<6)


struct zkcmTimerControlModS
{
	error_t (*initialize)(void);
	error_t (*shutdown)(void);
	error_t (*suspend)(void);
	error_t (*restore)(void);

	/**	EXPLANATION:
	 * Returns a bitmap of "safe" timer period values.
	 * The kernel uses this bit array to determine which timer period
	 * queues it should spawn for the chipset it is running on. The
	 * chipset defines this bitmap value in its chipset specific code.
	 * Bit values defined above as CHIPSET_TIMERS_*_SAFE.
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
	ubit32 (*getChipsetSafeTimerPeriods)(void);

};

#endif

