#ifndef _TIMER_TRIB_H
	#define _TIMER_TRIB_H

	#include <chipset/zkcm/zkcmIsr.h>
	#include <chipset/zkcm/timerDevice.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kclib/string.h>
	#include <__kclasses/clock_t.h>
	#include <__kclasses/singleWaiterQueue.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/multipleReaderLock.h>
	#include <kernel/common/processId.h>
	#include <kernel/common/messageStream.h>
	#include <kernel/common/timerTrib/timeTypes.h>
	#include <kernel/common/timerTrib/timerQueue.h>
	#include <kernel/common/timerTrib/timerStream.h>

#define TIMERTRIB				"TimerTrib: "

#define TIMERTRIB_WATCHDOG_ALREADY_REGISTERED	(1)

#define TIMERTRIB_TIMERQS_NQUEUES		(10)

class timerTribC
:
public tributaryC
{
friend class zkcmTimerControlModC;
friend class timerStreamC;
public:
	timerTribC(void);
	error_t initialize(void);
	~timerTribC(void);

public:
	status_t registerWatchdogIsr(zkcmIsrFn *, timeS interval);
	void updateWatchdogInterval(timeS interval);
	void unregisterWatchdogIsr(void);

	/**	Deprecated in lieu of redesign.
	void updateContinuousClock(void);
	void updateScheduledClock(uarch_t sourceId); */

	void getCurrentTime(timeS *t);
	void getCurrentDate(dateS *d);
	void getCurrentDateTime(timestampS *stamp);

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
	error_t enableWaitingOnQueue(timerQueueC *queue);
	error_t disableWaitingOnQueue(ubit32 nanos);
	error_t disableWaitingOnQueue(timerQueueC *queue);

	/**	Pending redesign.
	mstime_t	getCurrentTickMs(void);
	mstime_t	getUptimeTickMs(void);
	mstime_t	getTickIntervalSinceMs(mstime_t);

	void	setTimeoutMs(mstime_t, void (*)());
	void	setContinuousTimerMs(mstime_t, void (*)()); */

	void dump(void);

	// Artifacts from debugging.
	void sendMessage(void);
	void sendQMessage(void);

private:
	// Called by ZKCM Timer Control when a new timer device is detected.
	void newTimerDeviceNotification(zkcmTimerDeviceC *dev);

	/* Called by ZKCM Timer Control to ask the chipset for a mask of latched
	 * timer queues. This is only called by chipsets which emulate
	 * timekeeping in software (such as the IBM-PC).
	 *
	 * The chipset uses the bitmask of latched queues to determine which
	 * queue it would like to use to update the system clock. If for
	 * example, timekeeping on that chipset is best done to the accuracy of
	 * say, 1ms, the chipset would choose to use the 1ms period timer queue
	 * (if it's been latched) for timekeeping.
	 *
	 * The chipset will then install a routine on its chosen timer queue
	 * which will be called every time that queue tick()s. This routine is
	 * installed via installTimeKeeperRoutine().
	 *
	 * The kernel proper does not care whether or not timekeeping is being
	 * emulated in software; it simply assumes that some clock device on
	 * the chipset is keeping time, and that when it calls getCurrentTime(),
	 * the correct time will be returned.
	 **/
	ubit32 getLatchedTimerQueueMask(void) { return latchedPeriodMask; };

	error_t installClockRoutine(
		ubit32 chosenTimerQueue,
		// This typedef is declared in the timerQueueC header.
		zkcmTimerDeviceC::clockRoutineFn *routine);

	// Returns 1 if a routine was installed and actually removed.
	sarch_t uninstallClockRoutine(void);

	// Called by Timer Streams to add new Timer Request objects to timer Qs.
	error_t insertTimerQueueRequestObject(timerStreamC::requestS *request);
	// Called by Timer Streams to cancel Timer Request objects from Qs.
	sarch_t cancelTimerQueueRequestObject(timerStreamC::requestS *request);

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

	timerQueueC	period1s, period100ms, period10ms, period1ms,
			period100us, period10us, period1us,
			period100ns, period10ns, period1ns;
	timerQueueC	*timerQueues[TIMERTRIB_TIMERQS_NQUEUES];
	ubit32		safePeriodMask;
	ubit32		latchedPeriodMask;

	uarch_t		flags;
	sbit32		clockQueueId;
	sharedResourceGroupC<waitLockC, watchdogIsrS>	watchdog;

	// All of the event processing thread's state information.
	struct eventProcessorS
	{
		eventProcessorS(void)
		:
		tid(0), task(NULL)
		{
			memset(waitSlots, 0, sizeof(waitSlots));
		}

		// Entry point for the event processing thread.
		static void thread(void *);
		sarch_t getFreeWaitSlot(ubit8 *ret);
		void releaseWaitSlotFor(timerQueueC *timerQueue);
		void releaseWaitSlot(ubit8 slot)
		{
			waitSlots[slot].timerQueue = NULL;
			waitSlots[slot].eventQueue = NULL;
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
		void processExitMessage(messageS *);

		// PID and Pointer to event processing thread's taskC struct.
		processId_t		tid;
		threadC			*task;
		// Control queue used to send signals to the processing thread.
		singleWaiterQueueC	controlQueue;
		/* Array of wait queues for each of the timerQueues which are
		 * usable on this chipset. When a timer device is bound to a
		 * queue (newTimerDeviceNotification() -> initializeQueue()),
		 * a message is sent to the processing thread via the control
		 * queue, which causes the thread to begin checking that
		 * timerQueue's wait queue.
		 **/
		struct waitSlotS
		{
			timerQueueC		*timerQueue;
			singleWaiterQueueC	*eventQueue;
		} waitSlots[6];
	} eventProcessor;

private:
	void initializeQueue(timerQueueC *queue, ubit32 ns);
	void initializeAllQueues(void);
};

extern timerTribC		timerTrib;

#endif

