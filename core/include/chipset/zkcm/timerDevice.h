#ifndef _ZKCM_TIMER_SOURCE_H
	#define _ZKCM_TIMER_SOURCE_H

	#include <arch/atomic.h>
	#include <chipset/zkcm/device.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kflagManipulation.h>
	#include <__kclasses/singleWaiterQueue.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>
	#include <kernel/common/processId.h>
	#include <kernel/common/timerTrib/timeTypes.h>

/**	Constants used with struct zkcmTimerSourceS.
 **/
// Values for ZkcmTimerDevice.capabilities.modes.
#define ZKCM_TIMERDEV_CAP_MODE_PERIODIC		(1<<0)
#define ZKCM_TIMERDEV_CAP_MODE_ONESHOT		(1<<1)

// Values for ZkcmTimerDevice.state.flags.
/* Enabled is the state where the device is set to raise IRQs AND queue messages
 * on its IRQ event queue when its ISR is called to process one of its IRQs.
 **/
#define ZKCM_TIMERDEV_STATE_FLAGS_ENABLED	(1<<0)
#define ZKCM_TIMERDEV_STATE_FLAGS_LATCHED	(1<<1)
/* Soft enabled is the state where the device is set to raise IRQs, BUT /not/
 * queue messages on its IRQ event queue when its ISR is called to process
 * one of its IRQs.
 *
 * The primary use case for this is where a timer device is being used both for
 * the system clock, and for the Timer Tributary's timer queues. In this case,
 * the device cannot be hard disabled, because its IRQs must be used to update
 * the system clock; however, the Timer Tributary does not need IRQ event msgs
 * from the device because there are no timer service requests. The queueing
 * of IRQ event messages in this case is unoptimal because it wastes memory,
 * and causes the Timer Tributary's processing thread to be scheduled
 * unnecessarily.
 *
 * Thus, the "soft enabled" state is provided to enable the device to be told
 * to be hard enabled (keep raising IRQs and updating the system clock), but
 * "soft disabled" (don't bother the kernel with unnecessary IRQ event messages)
 **/
#define ZKCM_TIMERDEV_STATE_FLAGS_SOFT_ENABLED	(1<<2)

class ZkcmTimerDevice;

struct zkcmTimerEventS
{
	ZkcmTimerDevice		*device;
	class FloodplainnStream	*latchedStream;
	sTimestamp			irqStamp;
};

class ZkcmTimerDevice
:
public ZkcmDeviceBase
{
friend class TimerTrib;

public:
	enum timerTypeE { PER_CPU=0, CHIPSET };
	enum ioLatencyE { LOW=0, MODERATE, HIGH };
	enum precisionE { EXACT=0, NEGLIGABLE, OVERFLOW, UNDERFLOW };
	enum modeE { PERIODIC=0, ONESHOT, UNINITIALIZED };

	ZkcmTimerDevice(
		timerTypeE type, ubit32 modes,
		ubit32 periodicMinPeriod, ubit32 periodicMaxPeriod,
		ubit32 oneshotMinTimeout, ubit32 oneshotMaxTimeout,
		ioLatencyE ioLatency, precisionE precision,
		ZkcmDevice *device)
	:
	ZkcmDeviceBase(device),
	capabilities(
		type, modes,
		periodicMinPeriod, periodicMaxPeriod,
		oneshotMinTimeout, oneshotMaxTimeout,
		ioLatency, precision)
	{}

public:
	virtual error_t initialize(void);
	virtual error_t shutdown(void)=0;
	virtual error_t suspend(void)=0;
	virtual error_t restore(void)=0;

	virtual error_t enable(void)=0;
	error_t softEnable(void)
	{
		if (!isEnabled()) { return enable(); };

		state.lock.acquire();
		FLAG_SET(
			state.rsrc.flags,
			ZKCM_TIMERDEV_STATE_FLAGS_SOFT_ENABLED);

		state.lock.release();

		return ERROR_SUCCESS;
	}
		
	virtual void disable(void)=0;
	void softDisable(void)
	{
		state.lock.acquire();
		FLAG_UNSET(
			state.rsrc.flags,
			ZKCM_TIMERDEV_STATE_FLAGS_SOFT_ENABLED);

		state.lock.release();
	}

	sarch_t isEnabled(void)
	{
		uarch_t		flags;

		state.lock.acquire();
		flags = state.rsrc.flags;
		state.lock.release();

		return FLAG_TEST(flags, ZKCM_TIMERDEV_STATE_FLAGS_ENABLED);
	}

	sarch_t isSoftEnabled(void)
	{
		uarch_t		flags;

		state.lock.acquire();
		flags = state.rsrc.flags;
		state.lock.release();

		return FLAG_TEST(
			flags, ZKCM_TIMERDEV_STATE_FLAGS_SOFT_ENABLED);
	}
	// Call disable() before setting timer options, then enable() again.
	virtual status_t setPeriodicMode(struct sTime interval)=0;
	virtual status_t setOneshotMode(struct sTime timeout)=0;
	virtual void getOneshotModeMinMaxTimeout(sTime *min, sTime *max)
	{
		min->nseconds = capabilities.oneshotMinTimeout;
		max->nseconds = capabilities.oneshotMaxTimeout; 
	}
	
	virtual void getPeriodicModeMinMaxPeriod(sTime *min, sTime *max)
	{
		min->nseconds = capabilities.periodicMinPeriod;
		max->nseconds = capabilities.periodicMaxPeriod;
	}

	/* When a timer source has a 'capability.precision' other than
	 * EXACT or NEGLIGABLE, this API call will return the exact nanosecond
	 * underflow or overflow for the period passed as an argument.
	 *
	 * For example, on the IBM-PC, the RTC can generate a timing signal that
	 * has an /almost/ 1ms period, but underflows the exact precision of 1ms
	 * by a few fractions of a second. It actually generates a period of
	 * 0.976ms (0.976562). For this timer, the driver would set the
	 * "precision" property to PRECISION_UNDERFLOW, and when
	 * getPrecisionDisparityForPeriod(myTimer, ZKCM_TIMERDEV_PERIOD_1ms)
	 * is called, it would return the nanosecond disparity between the
	 * requested period and the actual period generated by the timer source.
	 **/
	virtual uarch_t getPrecisionDiscrepancyForPeriod(ubit32 period)=0;

	// Timekeeper call-in routine prototype, which chipsets must conform to.
	typedef void (clockRoutineFn)(ubit32 tickGranularity);
	virtual error_t installClockRoutine(clockRoutineFn *routine)
	{
		atomicAsm::set((uarch_t *)&clockRoutine, (uarch_t)routine);
		return ERROR_SUCCESS;
	}

	virtual sarch_t uninstallClockRoutine(void)
	{
		sarch_t		ret;

		ret = clockRoutine != NULL;
		atomicAsm::set((uarch_t *)&clockRoutine, (uarch_t)NULL);
		return ret;

	}

	SingleWaiterQueue *getEventQueue(void)
	{
		return &irqEventQueue;
	}

	error_t latch(class FloodplainnStream *stream);
	void unlatch(void);
	// Returns 1 if latched, 0 if not latched.
	sarch_t getLatchState(class FloodplainnStream **latchedStream);
	sarch_t validateCallerIsLatched(void);

public:

	struct sCapabilities
	{
		sCapabilities(
			timerTypeE type, ubit32 modes,
			ubit32 periodicMinPeriod, ubit32 periodicMaxPeriod,
			ubit32 oneshotMinTimeout, ubit32 oneshotMaxTimeout,
			ioLatencyE ioLatency, precisionE precision)
		:
		type(type), ioLatency(ioLatency), precision(precision),
		modes(modes),
		periodicMinPeriod(periodicMinPeriod),
		periodicMaxPeriod(periodicMaxPeriod),
		oneshotMinTimeout(oneshotMinTimeout),
		oneshotMaxTimeout(oneshotMaxTimeout)
		{}
		
		timerTypeE	type;
		ioLatencyE	ioLatency;
		precisionE	precision;
		// Modes (bitfield): PERIODIC, ONESHOT.
		ubit32		modes;
		ubit32		periodicMinPeriod, periodicMaxPeriod;
		ubit32		oneshotMinTimeout, oneshotMaxTimeout;
	} capabilities;

protected:
	zkcmTimerEventS *allocateIrqEvent(void)
	{
		return (zkcmTimerEventS *)irqEventCache->allocate();
	};

	void freeIrqEvent(zkcmTimerEventS *event)
	{
		irqEventCache->free(event);
	}

protected:
	struct sState
	{
		sState(void)
		:
		flags(0), latchedStream(0), mode(UNINITIALIZED), period(0)
		{}

		ubit32			flags;
		// Floodplain Stream for the process using this timer device.
		FloodplainnStream	*latchedStream;
		// Current mode: periodic/oneshot. Valid if FLAGS_ENABLED set.
		modeE			mode;
		// For periodic mode: stores the current timer period in ns.
		ubit32			period;
		// For oneshot mode: stores the current timeout date and time.
		sTime			currentTimeout;
		sTime			currentInterval;
	};

	SharedResourceGroup<WaitLock, sState>	state;
	SingleWaiterQueue			irqEventQueue;
	SlamCache				*irqEventCache;
	clockRoutineFn				*clockRoutine;
};

#endif

