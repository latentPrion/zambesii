#ifndef _IBM_PC_INTEL_8254_PIT_H
	#define _IBM_PC_INTEL_8254_PIT_H

	#include <extern.h>
	#include <__kstdlib/__ktypes.h>

CPPEXTERN_START

error_t ibmPc_i8254_initialize(void);
error_t ibmPc_i8254_shutdown(void);
error_t ibmPc_i8254_suspend(void);
error_t ibmPc_i8254_restore(void);

status_t ibmPc_i8254_enableTimerSource(struct zkcmTimerSourceS *timerSource);
void ibmPc_i8254_disableTimerSource(struct zkcmTimerSourceS *timerSource);

status_t ibmPc_i8254_setTimerSourcePeriodic(
	struct zkcmTimerSourceS *timerSource,
	struct timeS interval);

status_t ibmPc_i8254_setTimerSourceOneshot(
	struct zkcmTimerSourceS *timerSource,
	struct timestampS timeout);

uarch_t ibmPc_i8254_getPrecisionDiscrepancyForPeriod(
	struct zkcmTimerSourceS *timerSource,
	ubit32 period);


CPPEXTERN_END

#endif

