
#include <kernel/common/timerStream.h>


error_t timerStreamC::initialize(void)
{
	return events.initialize();
}

void timerStreamC::timerDeviceTimeoutNotification(zkcmTimerEventS *event)
{
	/**	EXPLANATION:
	 * The process' Timer Stream receives the timer event structure from
	 * the timer device's ISR. It queues the event object as needed, and
	 * then exits.
	 *
	 * It is now up to the process to call "pullTimerEvent()" to process the
	 * timer events that are pending on its stream.
	 *
	 * Returns void because there is really nothing that can be done if
	 * the memory allocation fails. We are currently executing in IRQ
	 * context, so the handler really can't be expected to properly know
	 * what to do with an error return value. At most we can panic, really.
	 *
	 **	TODO:
	 * The API for pulling from the event queue (pullTimerEvent()) has a
	 * "device" argument for filtering the events by device, such that the
	 * caller can ask only for events queued by a certain device. In order
	 * to facilitate this, the kernel needs to (optimally) have a separate
	 * queue instance for each timer device that the process has latched
	 * onto. This way, we can quickly pull events without any issues. An
	 * alternative would be to use prioQueueC, or a derived variant of it.
	 **/
	events.addItem(event);
}

error_t timerStreamC::pullTimerEvent(
	void */*device*/, ubit32 flags, zkcmTimerEventS *ret
	)
{
	/**	EXPLANATION:
	 * Blocking (or optionally non-blocking if DONT_BLOCK flag is passed)
	 * syscall to allow a process to pull expired timer events from its
	 * Timer Stream.
	 *
	 * Attempts to pull an event from "events" linked list. If it fails, it
	 * sleeps the process.
	 */
	return ERROR_SUCCESS;
}

