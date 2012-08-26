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

CPPEXTERN_END

// Declare the instance.
CPPEXTERN_PROTO struct zkcmTimerControlModS		ibmPc_timerControlMod;

#endif

