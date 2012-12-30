
#include <chipset/zkcm/timerControl.h>
#include <chipset/zkcm/timerControl_filters.h>
#include <__kstdlib/__kbitManipulation.h>


static ubit32 resolutionHigherFilterMasks[] =
{
	0xFFFFFFFF, 0xFFFFFFFE, 0xFFFFFFFC, 0xFFFFFFF8,
	0xFFFFFFF0, 0xFFFFFFE0, 0xFFFFFFC0, 0xFFFFFF80,
	0xFFFFFF00, 0xFFFFFE00, 0xFFFFFC00, 0xFFFFF800,
	0xFFFFF000, 0xFFFFE000, 0xFFFFC000, 0xFFFF8000
};

static ubit32 resolutionLowerFilterMasks[] =
{
	0x1, 0x3, 0x7, 0xF,
	0x1F, 0x3F, 0x7F, 0xFF,
	0x1FF, 0x3FF, 0x7FF, 0xFFF,
	0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF
};

sarch_t timerFilters::modes(
	zkcmTimerDeviceC *dev, ubit32 criteria, ubit32 flags
	)
{
	flags >>= TIMERCTL_FILTER_MODE_SHIFT;
	flags &= TIMERCTL_FILTER_MODE_MASK;
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

sarch_t timerFilters::resolutions(
	zkcmTimerDeviceC *dev, ubit32 criteria, ubit32 flags
	)
{
	ubit32		mask=0;

	if (criteria == 0) { return 0; };
	flags >>= TIMERCTL_FILTER_RES_SHIFT;
	flags &= TIMERCTL_FILTER_RES_MASK;

	if (flags == TIMERCTL_FILTER_RES_MATCH) {
		return dev->capabilities.resolutions & criteria;
	};

	if (flags == TIMERCTL_FILTER_RES_OR_HIGHER)
	{
		// Take the first bit that was set, and work with it.
		for (uarch_t i=0; i<8; i++)
		{
			if (__KBIT_TEST(criteria, i)) {
				mask = resolutionHigherFilterMasks[i];
			};
		};
	};

	if (flags == TIMERCTL_FILTER_RES_OR_LOWER)
	{
		// Take the first bit that was set and work with it.
		for (uarch_t i=0; i<8; i++)
		{
			if (__KBIT_TEST(criteria, i)) {
				mask = resolutionLowerFilterMasks[i];
			};
		};
	};

	return dev->capabilities.resolutions & mask;
}

sarch_t timerFilters::ioLatency(
	zkcmTimerDeviceC *dev, zkcmTimerDeviceC::ioLatencyE criteria,
	ubit32 flags
	)
{
	flags >>= TIMERCTL_FILTER_IO_SHIFT;
	flags &= TIMERCTL_FILTER_IO_MASK;

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
	flags >>= TIMERCTL_FILTER_PREC_SHIFT;
	flags &= TIMERCTL_FILTER_PREC_MASK;

	if (flags == TIMERCTL_FILTER_PREC_ANY) { return 1; };

	if (flags == TIMERCTL_FILTER_PREC_OR_BETTER) {
		return dev->capabilities.precision >= criteria;
	};

	if (flags == TIMERCTL_FILTER_PREC_OR_WORSE) {
		return dev->capabilities.precision <= criteria;
	};

	return 0;
}

