
#include <chipset/zkcm/timerControl.h>
#include <chipset/zkcm/timerSource.h>
#include <__kclasses/ptrList.h>

#include "timerControl.h"
#include "rtcmos.h"


ptrListC<zkcmTimerSourceS>	timerSources;

error_t ibmPc_timerControl_initialize(void)
{
	error_t		ret;

	ret = timerSources.initialize();
	if (ret != ERROR_SUCCESS) {
		return ret;
	};

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

struct zkcmTimerSourceS *ibmPc_timerControl_filterTimerSources(
	ubit8 type,		// PER_CPU or CHIPSET.
	ubit32 modes,		// PERIODIC | ONESHOT.
	ubit32 resolutions,	// 1s|100ms|10ms|1ms|100ns|10ns|1ns
	ubit8 ioLatency,	// LOW, MODERATE or HIGH.
	ubit8 precision,	// EXACT, NEGLIGABLE, OVERFLOW
				// or UNDERFLOW
	void **handle
	)
{
	zkcmTimerSourceS	*source;

	for (source = timerSources.getNextItem(handle);
		source != __KNULL;
		source = timerSources.getNextItem(handle))
	{
		// Must meet all of the criteria passed to us.
		if (source->capabilities.type != type) { continue; };
		if ((source->capabilities.modes & modes) != modes) {
			continue;
		};

		if ((source->capabilities.resolutions & resolutions)
			!= resolutions)
		{
			continue;
		};

		if (source->capabilities.ioLatency > ioLatency) { continue; };

		if (precision == ZKCM_TIMERSRC_CAP_PRECISION_ANY) {
			goto skipPrecisionCheck;
		};

		if (precision < ZKCM_TIMERSRC_CAP_PRECISION_OVERFLOW)
		{
			if (source->capabilities.precision > precision) {
				continue;
			};
		}
		else
		{
			/* The caller will only tolerate a timer that either
			 * overflows or underflows, as specified, so we have to
			 * give them a precision match that follows that
			 * request.
			 **/
			if (precision == ZKCM_TIMERSRC_CAP_PRECISION_OVERFLOW
				&& source->capabilities.precision
					> ZKCM_TIMERSRC_CAP_PRECISION_OVERFLOW)
			{
				continue;
			};

			if (precision == ZKCM_TIMERSRC_CAP_PRECISION_UNDERFLOW
				&& source->capabilities.precision
					== ZKCM_TIMERSRC_CAP_PRECISION_OVERFLOW)
			{
				continue;
			};

		};
skipPrecisionCheck:
		// If we got here, then the timer source matches.
		return source;
	};

	return __KNULL;
}

error_t ibmPc_irqControl_registerNewTimerSource(
	struct zkcmTimerSourceS *timerSource
	)
{
	// Ensure it's not already been added, then add it to the list.
	if (/*!timerSources.checkForItem(timerSource)*/ 0)
	{
	};

	return ERROR_SUCCESS;
}

