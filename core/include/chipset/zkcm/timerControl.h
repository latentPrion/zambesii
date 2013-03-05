#ifndef _ZKCM_TIMER_CONTROL_MODULE_H
	#define _ZKCM_TIMER_CONTROL_MODULE_H

	#include <chipset/zkcm/timerDevice.h>
	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/timerTrib/timeTypes.h>
	#include <kernel/common/processId.h>

	/**	EXPLANATION:
	 * The Timer Control module, part of the Zambesii Kernel Chipset
	 * Module API aggregate, is the interface that the kernel calls on to
	 * manage timer sources on a chipset. The kernel will
	 * directly manage timer devices within ZKCM. There is no
	 * plan to allow UDI drivers for timers on a chipset.
	 *
	 * The Timer Control mod will present the kernel with a list of timer
	 * sources and, if the kernel ever becomes interested, timer devices,
	 * which can be filtered based on programming I/O latency, supported
	 * resolutions, and any other criteria necessary. A timer device may
	 * provide more than one timer source.
	 **/

/**	Constants used with zkcmTimerControlModC and the Timer Control Mod API.
 **/
// Values for bitfield returned by getChipsetSafeTimerPeriods().
#define TIMERCTL_1S_SAFE		(1<<0)
#define TIMERCTL_100MS_SAFE		(1<<1)
#define TIMERCTL_10MS_SAFE		(1<<2)
#define TIMERCTL_1MS_SAFE		(1<<3)
#define TIMERCTL_100US_SAFE		(1<<4)
#define TIMERCTL_10US_SAFE		(1<<5)
#define TIMERCTL_1US_SAFE		(1<<6)
#define TIMERCTL_100NS_SAFE		(1<<7)
#define TIMERCTL_10NS_SAFE		(1<<8)
#define TIMERCTL_1NS_SAFE		(1<<9)

// Values for 'flags' argument of filterTimerDevices().
#define TIMERCTL_FILTER_MODE_SHIFT	(0)
#define TIMERCTL_FILTER_MODE_MASK	(0x3)
#define TIMERCTL_FILTER_MODE_EXACT	(0<<TIMERCTL_FILTER_MODE_SHIFT)
#define TIMERCTL_FILTER_MODE_BOTH	(1<<TIMERCTL_FILTER_MODE_SHIFT)
#define TIMERCTL_FILTER_MODE_ANY	(2<<TIMERCTL_FILTER_MODE_SHIFT)
#define TIMERCTL_FILTER_IO_SHIFT	(4)
#define TIMERCTL_FILTER_IO_MASK		(0x3)
#define TIMERCTL_FILTER_IO_OR_BETTER	(0<<TIMERCTL_FILTER_IO_SHIFT)
#define TIMERCTL_FILTER_IO_OR_WORSE	(1<<TIMERCTL_FILTER_IO_SHIFT)
#define TIMERCTL_FILTER_IO_ANY		(2<<TIMERCTL_FILTER_IO_SHIFT)
#define TIMERCTL_FILTER_PREC_SHIFT	(6)
#define TIMERCTL_FILTER_PREC_MASK	(0x3)
#define TIMERCTL_FILTER_PREC_OR_BETTER	(0<<TIMERCTL_FILTER_PREC_SHIFT)
#define TIMERCTL_FILTER_PREC_OR_WORSE	(1<<TIMERCTL_FILTER_PREC_SHIFT)
#define TIMERCTL_FILTER_PREC_ANY	(2<<TIMERCTL_FILTER_PREC_SHIFT)
#define TIMERCTL_FILTER_FLAGS_SKIP_LATCHED	(1<<16)

#define TIMERCTL_UNREGISTER_FLAGS_FORCE	(1<<0)

class zkcmTimerControlModC
{
public:

	error_t initialize(void);
	error_t shutdown(void);
	error_t suspend(void);
	error_t restore(void);

	/**	EXPLANATION:
	 * Returns a bitmap of "safe" timer period values.
	 * The kernel uses this bit array to determine which timer period
	 * queues it should spawn for the chipset it is running on. The
	 * chipset defines this bitmap value in its chipset specific code.
	 * Bit values are defined further up above as CHIPSET_TIMERS_*_SAFE.
	 *
	 * Not every chipset can operate stably with IRQs coming in every 1ms,
	 * for example. The chipset defines the bits which represent the timer
	 * periods which are /safe/ for models of that chipset. Generally though
	 * if a chipset model has a timer of a certain granularity, it was
	 * placed there by design, and the chipset was meant to process
	 * data that required timing at that frequency, so most chipsets will
	 * end up just defining all the frequencies supported by the timers on
	 * their model as safe.
	 **/
	ubit32 getChipsetSafeTimerPeriods(void);

	status_t getCurrentDate(dateS *date);
	status_t getCurrentTime(timeS *currTime);
	/* The chipset may cache the sytem time in RAM, and not actually be
	 * reading from the hardware clock on time API calls. This updates the
	 * the cached RAM value by reading from the hardware clock anew.
	 **/
	void refreshCachedSystemTime(void);
	/* The chipset may cache the system time in RAM, and not actually be
	 * updating the hardware clock when the system time value in RAM
	 * changes. This is especially so in situations where the hardware
	 * clock is imprecise, and the chipset is manually maintaining a
	 * system clock in RAM.
	 *
	 * This function flushes the cached RAM value to the hardware clock.
	 **/
	void flushCachedSystemTime(void);

	/**	EXPLANATION:
	 * This is the timer source search and filter API. It allows the caller
	 * to ask the timer control module to return all timer sources that
	 * match the criteria passed as arguments. This allows the caller
	 * to query the chipset for available timer sources that specifically
	 * meet its needs.
	 *
	 * Should no such timer sources be available, the caller may do another
	 * search with less stringent constraints, until an acceptable timer
	 * is found, or the caller determines that it will be unable to operate
	 * reliably on this chipset.
	 *
	 * Searching is done by calling filterTimerSources() repeatedly,
	 * passing a NULL handle on the first call, and subsequently passing
	 * the same handle back until the function returns NULL.
	 *
	 * See include/chipset/zkcm/timerSource.h for preprocessor constants.
	 **/
	zkcmTimerDeviceC *filterTimerDevices(
		zkcmTimerDeviceC::timerTypeE type,	// PER_CPU or CHIPSET.
		ubit32 modes,				// PERIODIC | ONESHOT.
		zkcmTimerDeviceC::ioLatencyE ioLatency,	// LOW, MODERATE or HIGH
		zkcmTimerDeviceC::precisionE precision,	// EXACT, NEGLIGABLE,
							// OVERFLOW or UNDERFLOW
		ubit32 flags,
		void **handle);

	/**	EXPLANATION:
	 * The chipset calls this function to register new timer sources as
	 * they are detected. They are then added to the list that is searchable
	 * by filterTimerSources.
	 **/
	error_t registerNewTimerDevice(zkcmTimerDeviceC *device);
	error_t unregisterTimerDevice(zkcmTimerDeviceC *device, uarch_t flags);
};

#endif

