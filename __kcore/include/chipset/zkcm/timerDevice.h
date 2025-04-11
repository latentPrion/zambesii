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
#define ZKCM_TIMERDEV_STATE_FLAGS_ENABLED   (1<<0)
#define ZKCM_TIMERDEV_STATE_FLAGS_LATCHED   (1<<1)
/* IRQ_EVENT_MESSAGES_ENABLED enables us to have a timer generating IRQs, but
 * not queueing messages on its IRQ event queue when its ISR is called to
 * process one of its IRQs.
 *
 * The primary use case for this is where the Timer Trib has no pending timer
 * service requests, but the device is being used both for the Timer Tributary's
 * timer queues and also for the system clock. In this case, the device's IRQs
 * cannot be hard disabled, because we need its IRQs to update the system clock.
 * However, the Timer Tributary does not need IRQ event msgs from the device
 * because there are no timer service requests. The queueing of IRQ event msgs
 * in this case is undesirable because it wastes CPU cycles and memory; and
 * causes the Timer Tributary's processing thread to be scheduled unnecessarily.
 *
 * Thus, the "IRQ_EVENT_MESSAGES_ENABLED" state is provided to enable the device
 * to be told to be hard enabled (keep raising IRQs and updating the system clock),
 * but to not enqueue IRQ event messages on its IRQ event queue in its ISR.
 **/
#define ZKCM_TIMERDEV_STATE_FLAGS_IRQ_EVENT_MESSAGES_ENABLED  (1<<2)

class ZkcmTimerDevice;

struct sZkcmTimerEvent
{
	ZkcmTimerDevice		*device;
	class FloodplainnStream *latchedStream;
	sTimestamp		irqStamp;
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
	ioLatency, precision),
	state(CC"ZkcmTimerDevice state")
	{}

public:
	virtual error_t initialize(void);
	virtual error_t shutdown(void);
	virtual error_t suspend(void)=0;
	virtual error_t restore(void)=0;

	virtual error_t enable(void)=0;

	/* See documentation for this feature flag above:
	 * ZKCM_TIMERDEV_STATE_FLAGS_IRQ_EVENT_MESSAGES_ENABLED
	 **/
	error_t enableIrqEventMessages(void)
	{
		// If the timer device is not enabled, enable it first.
		if (!isEnabled())
			{ return enable(); };

		/* Now indicate that the device's ISR should enqueue
		 * IRQ event msgs.
		 */
		state.lock.acquire();
		FLAG_SET(
			state.rsrc.flags,
			ZKCM_TIMERDEV_STATE_FLAGS_IRQ_EVENT_MESSAGES_ENABLED);

		state.lock.release();

		return ERROR_SUCCESS;
	}

	virtual void disable(void)=0;
	void disableIrqEventMessages(void)
	{
		state.lock.acquire();
		FLAG_UNSET(
			state.rsrc.flags,
			ZKCM_TIMERDEV_STATE_FLAGS_IRQ_EVENT_MESSAGES_ENABLED);

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

	sarch_t irqEventMessagesEnabled(void)
	{
		uarch_t		flags;

		state.lock.acquire();
		flags = state.rsrc.flags;
		state.lock.release();

		return FLAG_TEST(
			flags, ZKCM_TIMERDEV_STATE_FLAGS_IRQ_EVENT_MESSAGES_ENABLED);
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
		atomicAsm::set((uintptr_t *)&clockRoutine, (uintptr_t)routine);
		return ERROR_SUCCESS;
	}

	virtual sarch_t uninstallClockRoutine(void)
	{
		sarch_t		ret;

		ret = clockRoutine != NULL;
		atomicAsm::set((uintptr_t *)&clockRoutine, (uintptr_t)NULL);
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
	sZkcmTimerEvent *allocateIrqEvent(void)
	{
		return (sZkcmTimerEvent *)irqEventCache->allocate();
	};

	void freeIrqEvent(sZkcmTimerEvent *event)
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

