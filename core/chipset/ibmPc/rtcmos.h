#ifndef _IBM_PC_RTC_CMOS_H
	#define _IBM_PC_RTC_CMOS_H

	#include <extern.h>
	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/timerTrib/timeTypes.h>

CPPEXTERN_START

error_t ibmPc_rtc_initialize(void);
/*error_t ibmPc_rtc_shutdown(void);
error_t ibmPc_rtc_suspend(void);
error_t ibmPc_rtc_restore(void);*/

/*status_t ibmPc_rtc_getCurrentDate(dateS *date);
status_t ibmPc_rtc_getCurrentTime(timeS *time);*/
status_t ibmPc_rtc_getHardwareDate(dateS *ret);
status_t ibmPc_rtc_getHardwareTime(timeS *ret);
/*void ibmPc_rtc_refreshCachedSystemTime(void);
void ibmPc_rtc_flushCachedSystemTime(void); */

CPPEXTERN_END

#endif

