
#include <chipset/zkcm/timerControl.h>
#include <chipset/zkcm/timerControl_filters.h>
#include <__kstdlib/__kbitManipulation.h>


sarch_t timerFilters::modes(
	zkcmTimerDeviceC *dev, ubit32 criteria, ubit32 flags
	)
{
	flags &= TIMERCTL_FILTER_MODE_MASK << TIMERCTL_FILTER_MODE_SHIFT;

	if (criteria == 0 && flags != TIMERCTL_FILTER_MODE_ANY) {
		return 0;
	};

	if (flags == TIMERCTL_FILTER_MODE_ANY) { return 1; };
	if (flags == TIMERCTL_FILTER_MODE_BOTH)
	{
		// Only if both modes are available.
		return dev->capabilities.modes ==
			(ZKCM_TIMERDEV_CAP_MODE_PERIODIC
			| ZKCM_TIMERDEV_CAP_MODE_ONESHOT);
	};

	if (flags == TIMERCTL_FILTER_MODE_EXACT)
	{
		// Only if the exact mode requested is available.
		return (dev->capabilities.modes & criteria) == criteria;
	};

	return 0;
}

sarch_t timerFilters::ioLatency(
	zkcmTimerDeviceC *dev, zkcmTimerDeviceC::ioLatencyE criteria,
	ubit32 flags
	)
{
	flags &= TIMERCTL_FILTER_IO_MASK << TIMERCTL_FILTER_IO_SHIFT;

	if (flags == TIMERCTL_FILTER_IO_ANY) { return 1; };

	if (flags == TIMERCTL_FILTER_IO_OR_BETTER) {
		return dev->capabilities.ioLatency >= criteria;
	};

	if (flags == TIMERCTL_FILTER_IO_OR_WORSE) {
		return dev->capabilities.ioLatency <= criteria;
	};

	return 0;
}

sarch_t timerFilters::precision(
	zkcmTimerDeviceC *dev, zkcmTimerDeviceC::precisionE criteria,
	ubit32 flags
	)
{
	flags &= TIMERCTL_FILTER_PREC_MASK << TIMERCTL_FILTER_PREC_SHIFT;

	if (flags == TIMERCTL_FILTER_PREC_ANY) { return 1; };

	if (flags == TIMERCTL_FILTER_PREC_OR_BETTER) {
		return dev->capabilities.precision >= criteria;
	};

	if (flags == TIMERCTL_FILTER_PREC_OR_WORSE) {
		return dev->capabilities.precision <= criteria;
	};

	return 0;
}

