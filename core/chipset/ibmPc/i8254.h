#ifndef _IBM_PC_INTEL_8254_PIT_H
	#define _IBM_PC_INTEL_8254_PIT_H

	#include <chipset/zkcm/zkcmIsr.h>
	#include <chipset/zkcm/timerDevice.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/cachePool.h>

#define i8254			"IBMPC-8254: "

/* See comments in validateOneshotTimevalLimit() to understand the need for
 * these values, and how they were derived. Feel free to submit more accurate
 * calculations too.
 *
 * Minimum resolution period is set to 1ms, max is set to 54.928ms. Even though
 * the i8254 is technically capable of supporting microsecond resolution, we
 * don't allow it, and treat it as if it has a millisecond resolution clock
 * source, because programming the i8254 to count down microsecond values
 * causes spurious IRQs on some hardware. I have not yet seen any hardware cases
 * where programming it to count a millisecond granular time val has triggered
 * strange behaviour.
 **/
#define i8254_MIN_NS			(1000000)
#define i8254_MAX_NS			(54928000)
/* These values are explained in i8254.cpp. Conversion value for transforming
 * microseconds into i8254 CLK pulses (hence the "US2CLK").
 **/
#define i8254_US2CLK_10K		(11932)
#define i8254_US2CLK_1K			(1193)
#define i8254_US2CLK_100		(119)
#define i8254_US2CLK_10			(12)
#define i8254_US2CLK_1			(1)

class i8254PitC
:
public zkcmTimerDeviceC
{
public:
	explicit i8254PitC(ubit32 childId)
	:
	zkcmTimerDeviceC(
		zkcmTimerDeviceC::CHIPSET,
		ZKCM_TIMERDEV_CAP_MODE_PERIODIC
			| ZKCM_TIMERDEV_CAP_MODE_ONESHOT,
		i8254_MIN_NS, i8254_MAX_NS,
		i8254_MIN_NS, i8254_MAX_NS,
		zkcmTimerDeviceC::MODERATE,
		zkcmTimerDeviceC::NEGLIGABLE,
		&baseDeviceInfo),
	baseDeviceInfo(
		childId,
		CC"i8254", CC"IBM-PC compatible i8254 PIT, channel 0 timer.",
		CC"Unknown vendor", CC"N/A")
	{}

public:
	virtual error_t initialize(void);
	virtual error_t shutdown(void);
	virtual error_t suspend(void) { return ERROR_SUCCESS; }
	virtual error_t restore(void) { return ERROR_SUCCESS; }

	virtual status_t enable(void);
	virtual void disable(void);
	// Call disable() before setting timer options, then enable() again.
	virtual status_t setPeriodicMode(struct timeS interval);
	virtual status_t setOneshotMode(struct timeS timeout);
	virtual uarch_t getPrecisionDiscrepancyForPeriod(ubit32 /*period*/) { return 0; };

	// IRQ Handler ISR for the i8254 PIT.
	static zkcmIsrFn	isr;

private:
	void sendEoi(void);
	void writeOneshotIo(void);
	void writePeriodicIo(void);

private:
	zkcmDeviceC		baseDeviceInfo;

	struct i8254StateS
	{
		enum irqStateE { DISABLED=1, ENABLED, DISABLING };
		i8254StateS(void)
		:
		currentTimeoutClks(0), currentIntervalClks(0),
		isrRegistered(0), __kpinId(0), irqState(DISABLED)
		{}

		// i8254 CLK equivalent of nanosecond value in currentTimeout.
		ubit16			currentTimeoutClks;
		ubit16			currentIntervalClks;
		// Boolean to determine whether or not our ISR is registered.
		ubit8			isrRegistered;
		// __kpin ID of the IRQ pin our device is currently tied to.
		ubit16			__kpinId;
		irqStateE		irqState;
	} i8254State;
};

extern i8254PitC	i8254Pit;

#endif

