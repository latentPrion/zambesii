
#include <__kstdlib/__kclib/string.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/timerTrib/timerQueue.h>


/**	EXPLANATION:
 * Timer Queue is the class used to represent queues of sorted timer request
 * objects from processes (kernel, driver and user). At boot, the kernel will
 * check to see how many timer sources the chipset provides, and how many of
 * them can be used to initialize timer queues. Subsequently, if any new
 * timer queues are detected, the kernel will automatically attempt to see if
 * it can be used to initialize any still-uninitialized timer queues.
 *
 * Any left-over timer sources are exposed to any process which may want to use
 * them via the ZKCM Timer Control mod's TimerFilter api.
 *
 * The number of Timer Queues that the kernel initializes for a chipset is
 * of course, chipset dependent. It is based on the chipset's "safe period bmp",
 * which states what timer periods the chipset can handle safely. Not every
 * chipset can have a timer running at 100Hz and remain stable, for example.
 **/

error_t timerQueueC::latch(zkcmTimerDeviceC *dev)
{
	error_t		ret;

	if (dev == __KNULL) { return ERROR_INVALID_ARG; };

	ret = dev->latch(&processTrib.__kprocess.floodplainnStream);
	if (ret != ERROR_SUCCESS)
	{
		__kprintf(WARNING TIMERQUEUE"%dus: latch: to dev \"%s\" failed."
			"\n", nativePeriod / 1000,
			dev->getBaseDevice()->shortName);

		return ret;
	};

	device = dev;
	__kprintf(NOTICE TIMERQUEUE"%dus: latch: latched to device \"%s\".\n",
		getNativePeriod() / 1000, device->getBaseDevice()->shortName);

	return ERROR_SUCCESS;
}

void timerQueueC::unlatch(void)
{
	if (!isLatched()) { return; };

	disable();
	device->unlatch();
	device = __KNULL;

	__kprintf(NOTICE TIMERQUEUE"%dus: unlatch: unlatched device \"%s\".\n",
		getNativePeriod() / 1000);
}

error_t timerQueueC::enable(void)
{
	error_t		ret;
	timeS		stamp;

	if (!isLatched()) {
		return ERROR_UNINITIALIZED;
	};

	ret = device->setPeriodicMode(timeS(0, nativePeriod));
	if (ret != ERROR_SUCCESS)
	{
		__kprintf(ERROR TIMERQUEUE"%dus: Failed to set periodic mode "
			"on \"%s\".\n",
			getNativePeriod() / 1000,
			getDevice()->getBaseDevice()->shortName);

		return ret;
	};

	ret = device->enable();
	if (ret != ERROR_SUCCESS)
	{
		__kprintf(ERROR TIMERQUEUE"%dus: Failed to enable() device.\n",
			getNativePeriod() / 1000);

		return ret;
	};

	return ERROR_SUCCESS;
}

void timerQueueC::disable(void)
{
	/** FIXME: Should first check to see if there are objects left, and wait
	 * for them to expire before physically disabling the timer source.
	 **/
	if (!isLatched()) { return; };
	device->disable();
}

error_t timerQueueC::insert(timerObjectS *obj)
{
	error_t		ret;

	ret = requestQueue.addItem(obj, obj->expirationStamp);
	if (ret != ERROR_SUCCESS) { return ret; };

	if (requestQueue.getNItems() == 1) {
		return enable();
	};

	return ERROR_SUCCESS;
}

sarch_t timerQueueC::cancel(timerObjectS *obj)
{
	/**	FIXME:
	 * This function is intended to return 1 if the item being canceled was
	 * truly canceled (it had not yet expired and been processed), or 0 if
	 * it had already been processed and the "cancel" operation ended up
	 * being a NOP.
	 *
	 * Sadly, the list class doesn't return a value from removeItem().
	 **/
	requestQueue.removeItem(obj);
	return 1;
}

extern int breakOut;
int nTicks=0;
void timerQueueC::tick(zkcmTimerEventS *event)
{
	timerObjectS		*obj;

	/**	EXPLANATION
	 * Get the request at the front of the queue, and if it's expired,
	 * queue an event on the originating process' Timer Stream.
	 *
	 * If the queue is emptied by the sequence, disable the underlying
	 * timer device.
	 **/
	obj = requestQueue.getHead();
	if (obj == __KNULL)
	{
		__kprintf(WARNING TIMERQUEUE"%dus: tick called on empty Q.\n",
			getNativePeriod() / 1000);

		return;
	};

nTicks++;
__kprintf(NOTICE"Here 4. This means we're inside tick().\n");
	if (/*obj->expirationStamp <= event->irqStamp*/ nTicks >500)
	{
		__kprintf(NOTICE TIMERQUEUE"%dus: Event just expired.\n",
			getNativePeriod() / 1000);

		disable();
		breakOut=1;
	};
}

