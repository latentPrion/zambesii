#ifndef _TIMER_TRIB_H
	#define _TIMER_TRIB_H

	#include <chipset/zkcm/zkcmIsr.h>
	#include <chipset/zkcm/timerDevice.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/clock_t.h>
	#include <__kclasses/singleWaiterQueue.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/multipleReaderLock.h>
	#include <kernel/common/processId.h>
	#include <kernel/common/timerTrib/timeTypes.h>
	#include <kernel/common/timerTrib/timerQueue.h>
	#include <kernel/common/timerTrib/timerStream.h>

#define TIMERTRIB				"TimerTrib: "

#define TIMERTRIB_WATCHDOG_ALREADY_REGISTERED	(1)

class timerTribC
:
public tributaryC
{
friend void ::__korientationMain(void);
public:
	timerTribC(void);
	error_t initialize(void);
	error_t initialize2(void);
	~timerTribC(void);

public:
	status_t registerWatchdogIsr(zkcmIsrFn *, timeS interval);
	void updateWatchdogInterval(timeS interval);
	void unregisterWatchdogIsr(void);

	// Called by ZKCM Timer Control when a new timer device is detected.
	void newTimerDeviceNotification(zkcmTimerDeviceC *dev);

	/**	Deprecated in lieu of redesign.
	void updateContinuousClock(void);
	void updateScheduledClock(uarch_t sourceId); */

	void getCurrentTime(timeS *t);
	date_t getCurrentDate(void);

	/**	EXPLANATION:
	 * Called by timer device drivers from IRQ context to let the kernel
	 * know that a timer device's programmed timeout has been reached. The
	 * kernel will in queue an event object on the Timer Stream of the
	 * process that is currently latched to the timer device that has IRQ'd.
	 *
	 * It is then the responsibility of the latched process to call
	 * "pullTimerEvent()" on its Timer Stream so that it can read timer
	 * events and respond to them.
	 **/
	// void timerDeviceTimeoutEvent(zkcmTimerDeviceC *dev);

	/**	Input value is any multiple of 10 up to 1,000,000,000, where
	 * "1" indicates 1ns and 1,000,000,000 represents 1 second. So, to
	 * enable the 1millisecond queue, you would pass "1000000" as the arg.
	 *
	 * Returns ERROR_SUCCESS on success,
	 *	ERROR_UNINITIALIZED if the queue was unable to get a suitable
	 *		timer source to latch onto,
	 *	ERROR_NOT_SUPPORTED if the queue is not supported by the
	 *		chipset.
	 *	ERROR_INVALID_ARG_VAL if an invalid arg is supplied.
	 **/
	error_t enableQueue(ubit32 nanos);
	error_t disableQueue(ubit32 nanos);

	/**	Pending redesign.
	mstime_t	getCurrentTickMs(void);
	mstime_t	getUptimeTickMs(void);
	mstime_t	getTickIntervalSinceMs(mstime_t);

	void	setTimeoutMs(mstime_t, void (*)());
	void	setContinuousTimerMs(mstime_t, void (*)()); */

	void dump(void);
	static void eventProcessorThread(void);

private:
	// The watchdog timer for the chipset, if it exists.
	struct watchdogIsrS
	{
		zkcmIsrFn	*isr;
		timestampS	nextFeedTime;
		timeS		interval;
	};

	struct eventProcessorMessageS
	{
		enum		typeE
		{
			QUEUE_ENABLED=1,
			QUEUE_DISABLED,
			EXIT_THREAD
		};

		ubit8		type;
		timerQueueC	*timerQueue;
	};

	// Timestamp value for when this kernel instance was booted.
	timestampS	bootTimestamp;
	timerQueueC	period1s, period100ms, period10ms, period1ms;
	ubit32		safePeriodMask;
	uarch_t		flags;
	sharedResourceGroupC<waitLockC, watchdogIsrS>	watchdog;
	taskC		*eventProcessorTask;
	singleWaiterQueueC<eventProcessorMessageS>
		eventProcessorControlQueue;

private:
	void initializeQueue(timerQueueC *queue, ubit32 ns);
	void initializeAllQueues(void);
};

extern timerTribC		timerTrib;

#endif

