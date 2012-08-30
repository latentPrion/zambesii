
#include <chipset/zkcm/timerControl.h>
#include "timerControl.h"
#include "rtcmos.h"


error_t ibmPc_timerControl_initialize(void)
{
	return ibmPc_rtc_initialize();
}

error_t ibmPc_timerControl_shutdown(void)
{
	return ERROR_SUCCESS;
}

error_t ibmPc_timerControl_suspend(void)
{
	return ERROR_SUCCESS;
}

error_t ibmPc_timerControl_restore(void)
{
	return ERROR_SUCCESS;
}

ubit32 ibmPc_timerControl_getChipsetSafeTimerPeriods(void)
{
	/** EXPLANATION:
	 * For IBM-PC, safe timer periods are 1s, 100ms, 10ms and 1ms. The
	 * kernel won't support nanosecond resolution on the IBM-PC until the
	 * industry has securely moved in that direction.
	 **/
	return CHIPSET_TIMERS_1S_SAFE
		| CHIPSET_TIMERS_100MS_SAFE | CHIPSET_TIMERS_10MS_SAFE
		| CHIPSET_TIMERS_1MS_SAFE;
}

