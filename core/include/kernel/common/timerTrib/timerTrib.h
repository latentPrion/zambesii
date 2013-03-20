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
	void getCurrentDate(dateS *d);

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
	error_t enableWaitingOnQueue(ubit32 nanos);
	error_t disableWaitingOnQueue(ubit32 nanos);

	/**	Pending redesign.
	mstime_t	getCurrentTickMs(void);
	mstime_t	getUptimeTickMs(void);
	mstime_t	getTickIntervalSinceMs(mstime_t);

	void	setTimeoutMs(mstime_t, void (*)());
	void	setContinuousTimerMs(mstime_t, void (*)()); */

	void dump(void);

private:
	// The watchdog timer for the chipset, if it exists.
	struct watchdogIsrS
	{
		zkcmIsrFn	*isr;
		timestampS	nextFeedTime;
		timeS		interval;
	};


	// Timestamp value for when this kernel instance was booted.
	timestampS	bootTimestamp;
	timerQueueC	period1s, period100ms, period10ms, period1ms;
	ubit32		safePeriodMask;
	uarch_t		flags;
	sharedResourceGroupC<waitLockC, watchdogIsrS>	watchdog;

	// The event processing thread's state information.
	struct eventProcessorS
	{
		eventProcessorS(void)
		:
		tid(0), task(__KNULL)
		{
			memset(waitSlots, 0, sizeof(waitSlots));
		}

		// Entry point for the event processing thread.
		static void thread(void *);
		sarch_t getFreeWaitSlot(ubit8 *ret);
		void releaseWaitSlotFor(timerQueueC *timerQueue);
		void releaseWaitSlot(ubit8 slot)
		{
			waitSlots[slot].timerQueue = __KNULL;
			waitSlots[slot].eventQueue = __KNULL;
		}

		struct messageS
		{
			enum typeE {
				QUEUE_LATCHED=1, QUEUE_UNLATCHED, EXIT_THREAD };

			messageS(void)
			:
			type(static_cast<typeE>( 0 ))
			{}

			messageS(typeE type, timerQueueC *timerQueue)
			:
			type(type), timerQueue(timerQueue)
			{}

			typeE		type;
			timerQueueC	*timerQueue;
		};

		void processQueueLatchedMessage(messageS *msg);
		void processQueueUnlatchedMessage(messageS *msg);
		void processExitMessage(messageS *)
		{
			UNIMPLEMENTED(
				"timerTribC::"
				"eventProcessorS::processExitMessage");
		}

		// PID and Pointer to event processing thread's taskC struct.
		processId_t	tid;
		taskC		*task;
		// Control queue used to send signals to the processing thread.
		singleWaiterQueueC<messageS>	controlQueue;
		/* Array of wait queues for each of the timerQueues which are
		 * usable on this chipset. When a timer device is bound to a
		 * queue (newTimerDeviceNotification() -> initializeQueue()),
		 * a message is sent to the processing thread via the control
		 * queue, which causes the thread to begin checking that
		 * timerQueue's wait queue.
		 **/
		struct waitSlotS
		{
			timerQueueC	*timerQueue;
			singleWaiterQueueC<zkcmTimerEventS>	*eventQueue;
		} waitSlots[6];
	} eventProcessor;

private:
	void initializeQueue(timerQueueC *queue, ubit32 ns);
	void initializeAllQueues(void);
};

extern timerTribC		timerTrib;

#endif

