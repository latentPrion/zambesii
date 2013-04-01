#ifndef _TIMER_QUEUE_H
	#define _TIMER_QUEUE_H

	#include <chipset/zkcm/timerDevice.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/sortedPtrDoubleList.h>
	#include <kernel/common/timerTrib/timeTypes.h>

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
	 * are generated via syscalls to the Timer Streams; a process can have
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

#define TIMERQUEUE		"timerQ "

struct zkcmTimerEventS;
class timerTribC;

class timerQueueC
{
friend void ::__korientationMain(void);
friend class timerTribC;
private:
	timerQueueC(ubit32 nativePeriod)
	:
	currentPeriod(nativePeriod), nativePeriod(nativePeriod),
	device(__KNULL)
	{}

	error_t initialize(void) { return requestQueue.initialize(); }
	~timerQueueC(void) {}

private:
	error_t latch(zkcmTimerDeviceC *device);
	void unlatch(void);
	sarch_t isLatched(void) { return device != __KNULL; };
	zkcmTimerDeviceC *getDevice(void) { return device; };

	error_t insert(timerObjectS *obj);
	sarch_t cancel(timerObjectS *obj);

	ubit32 getCurrentPeriod(void) { return currentPeriod; };
	status_t setCurrentPeriod(ubit32 p) { currentPeriod = p; return 0; };
	ubit32 getNativePeriod(void) { return nativePeriod; };
	status_t setNativePeriod(ubit32 p) { nativePeriod = p; return 0; };

	/* Called by the kernel to ask this timer queue if the device "dev" is
	 * suitable for use with this timer queue.
	 **/
	sarch_t testTimerDeviceSuitability(zkcmTimerDeviceC *dev)
	{
		timeS		min, max;

		dev->getPeriodicModeMinMaxPeriod(&min, &max);
		if (nativePeriod < min.nseconds || nativePeriod > max.nseconds)
		{
			return 0;
		};

		return 1;
	}

	/* Called by the Timer Trib Dqer thread when the latched timer for this
	 * instance fires an IRQ. Expires/migrates the items at the front of the
	 * Q if necessary, and sets the pending event bit in the process'
	 * PCB, before waking the thread that registered to listen for the
	 * event (if any).
	 **/
	void tick(zkcmTimerEventS *timerIrqEvent);

	/* Enables or disables the queue's underlying timer source. On
	 * call to disable(), if there are objects waiting to be timed out and
	 * dispatched on the queue, they will be allowed to time out and trigger
	 * before the queue is disabled; however the queue will not take any
	 * further requests for timer services until enable() is called, even
	 * if it is still waiting for stale requests to time out.
	 **/
	error_t enable(void);
	void disable(void);

private:
	// Specified in nanoseconds.
	ubit32		currentPeriod, nativePeriod;

	// The actual internal queue instance for timer request objects.
	sortedPointerDoubleListC<timerObjectS, timestampS>	requestQueue;
	zkcmTimerDeviceC	*device;
};

#endif

