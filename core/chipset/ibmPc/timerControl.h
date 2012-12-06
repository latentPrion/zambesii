#ifndef _IBM_PC_TIMER_CONTROL_MODULE_H
	#define _IBM_PC_TIMER_CONTROL_MODULE_H

	#include <extern.h>
	#include <chipset/zkcm/timerControl.h>
	#include <__kstdlib/__ktypes.h>

#define IBMPCTIMERCTL		"Timer-Ctl: "

CPPEXTERN_START

error_t ibmPc_timerControl_initialize(void);
error_t ibmPc_timerControl_shutdown(void);
error_t ibmPc_timerControl_suspend(void);
error_t ibmPc_timerControl_restore(void);

ubit32 ibmPc_timerControl_getChipsetSafeTimerPeriods(void);
struct zkcmTimerSourceS *ibmPc_timerControl_filterTimerSources(
	ubit8 type,		// PER_CPU or CHIPSET.
	ubit32 modes,		// PERIODIC | ONESHOT.
	ubit32 resolutions,	// 1s|100ms|10ms|1ms|100ns|10ns|1ns
	ubit8 ioLatency,	// LOW, MODERATE or HIGH.
	ubit8 precision,	// EXACT, NEGLIGABLE, OVERFLOW
				// or UNDERFLOW
	void **handle);


CPPEXTERN_END

// Declare the instance.
CPPEXTERN_PROTO struct zkcmTimerControlModC		ibmPc_timerControlMod;

#endif

