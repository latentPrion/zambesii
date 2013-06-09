
#include <__kstdlib/__kflagManipulation.h>
#include <kernel/common/callbackStream.h>
#include <kernel/common/taskTrib/taskTrib.h>


error_t callbackStreamC::pull(headerS **callback, ubit32 flags)
{
	if (callback == __KNULL) { return ERROR_INVALID_ARG; };

	for (;;)
	{
		pendingSubsystems.lock();

		for (ubit16 i=0; i<ZCALLBACK_SUBSYSTEM_MAXVAL + 1; i++)
		{
			if (pendingSubsystems.test(i))
			{
				*callback = queues[i].popFromHead();
				if (queues[i].getNItems() == 0) {
					pendingSubsystems.unset(i);
				};

				pendingSubsystems.unlock();
				return ERROR_SUCCESS;
			};
		};

		if (__KFLAG_TEST(flags, ZCALLBACK_PULL_FLAGS_DONT_BLOCK))
		{
			pendingSubsystems.unlock();
			return ERROR_WOULD_BLOCK;
		};

		taskTrib.block(
			&pendingSubsystems.bmp.lock,
			TASKTRIB_BLOCK_LOCKTYPE_WAIT);
	};
}

error_t	callbackStreamC::enqueue(headerS *callback)
{
	error_t		ret;

	if (callback == __KNULL) { return ERROR_INVALID_ARG; };
	if (callback->subsystem > ZCALLBACK_SUBSYSTEM_MAXVAL) {
		return ERROR_INVALID_ARG_VAL;
	};

	/**	TODO:
	 * Think about this type of situation and determine whether or not it's
	 * safe to execute within the critical section with interrupts enabled
	 * on the local CPU. Most likely not though.
	 **/
	pendingSubsystems.lock();

	ret = queues[callback->subsystem].addItem(callback);
	if (ret == ERROR_SUCCESS)
	{
		pendingSubsystems.set(callback->subsystem);
		// Unblock the thread.
		taskTrib.unblock(parent);
	};

	pendingSubsystems.unlock();

	return ret;
}

