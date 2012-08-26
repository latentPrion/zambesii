#ifndef _TIMER_QUEUE_H
	#define _TIMER_QUEUE_H

	#include <__kstdlib/__ktypes.h>

	/**	EXPLANATION:
	 * The subject of Zambesii timer source management as a whole will
	 * deliberately not be discussed here.
	 *
	 * Each instance of the timerQueueC class represents a single queue
	 * within the Timer Trib. Each queue has a native period, and a 
	 * current period.
	 *
	 * Each instantiated object must have a real hardware timer IRQ source
	 * bound to it. A queue holds multiple timing service request objects,
	 * all of which have an expiration time filled into them. These requests
	 * are generated via syscalls to the Timer Tributary; a process can have
	 * any number of pending timer requests, but the kernel will only allow
	 * a single request from each process to actually be in the timer queues
	 * at any given time. The rest of a process's timer requests are left on
	 * its Timer Stream.
	 *
	 * Each queue, as stated above has a real hardware timer IRQ source
	 * to which is it bound. When that timer source fires an IRQ, the IRQ
	 * handler for that IRQ source is expected to call the Timer Trib
	 * and pass it information needed to identify that timer source. If the
	 * timer source's ID info matches that of a source used by any queue,
	 * the first object at the front of that queue is then checked
	 * immedately for expiration.
	 *
	 * The frequency at which the timer source that pertains to a queue
	 * fires its IRQ is dependent on the queue's "currentPeriod", a value
	 * in nanoseconds. Each queue has a "nativePeriod" which it strives to
	 * drive its timer source at, but by design, a queue may modify the
	 * frequency of its assigned timer to meet timing discrepancies on the
	 * board.
	 **/

class timerQueueC
{
private:
	ubit32		currentPeriod, nativePeriod;
};

#endif

