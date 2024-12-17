#ifndef _TIMER_CONTROL_FILTERS_H
	#define _TIMER_CONTROL_FILTERS_H

	#include <chipset/zkcm/timerDevice.h>
	#include <__kstdlib/__ktypes.h>

namespace timerFilters
{
	sarch_t modes(ZkcmTimerDevice *dev, ubit32 criteria, ubit32 flags);
	sarch_t resolutions(
		ZkcmTimerDevice *dev, ubit32 criteria, ubit32 flags);

	sarch_t ioLatency(
		ZkcmTimerDevice *dev, ZkcmTimerDevice::ioLatencyE criteria,
		ubit32 flags);

	sarch_t precision(
		ZkcmTimerDevice *dev, ZkcmTimerDevice::precisionE criteria,
		ubit32 flags);
}

#endif

