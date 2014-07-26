#ifndef _TIMER_CONTROL_FILTERS_H
	#define _TIMER_CONTROL_FILTERS_H

	#include <chipset/zkcm/timerDevice.h>
	#include <__kstdlib/__ktypes.h>

namespace timerFilters
{
	sarch_t modes(zkcmTimerDeviceC *dev, ubit32 criteria, ubit32 flags);
	sarch_t resolutions(
		zkcmTimerDeviceC *dev, ubit32 criteria, ubit32 flags);

	sarch_t ioLatency(
		zkcmTimerDeviceC *dev, zkcmTimerDeviceC::ioLatencyE criteria,
		ubit32 flags);

	sarch_t precision(
		zkcmTimerDeviceC *dev, zkcmTimerDeviceC::precisionE criteria,
		ubit32 flags);
}

#endif

