
#include <__kstdlib/__kflagManipulation.h>
#include <kernel/common/callbackStream.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/processTrib/processTrib.h>

error_t callbackStreamC::enqueueCallback(
	processId_t targetStreamId, zcallback::headerS *header
	)
{
	callbackStreamC	*targetStream;

	if (__KFLAG_TEST(header->flags, ZCALLBACK_FLAGS_CPU_TARGET))
	{
		cpuStreamC		*cs;

		/* Dealing with an asynchronous response to an API call from a
		 * per-CPU thread.
		 **/
		cs = cpuTrib.getStream((cpu_t)targetStreamId);
		if (cs == __KNULL) { return ERROR_INVALID_RESOURCE_NAME; };

		targetStream = &cs->getTaskContext()->callbackStream;
	}
	else
	{
		processStreamC		*targetProcess;
		threadC			*targetThread;

		/* Dealing with an asynchronous response to an API call from a
		 * normal unique-context thread.
		 **/
		targetProcess = processTrib.getStream(targetStreamId);
		if (targetProcess == __KNULL) {
			return ERROR_INVALID_RESOURCE_NAME;
		};

		targetThread = targetProcess->getThread(targetStreamId);
		if (targetThread == __KNULL) {
			return ERROR_INVALID_RESOURCE_NAME;
		};

		targetStream = &targetThread->getTaskContext()->callbackStream;
	};

	return targetStream->enqueue(header);
}

error_t callbackStreamC::pull(
	zcallback::headerS **callback, ubit32 flags
	)
{
	if (callback == __KNULL) { return ERROR_INVALID_ARG; };

	for (;;)
	{
		pendingSubsystems.lock();

		for (ubit16 i=0; i<ZMESSAGE_SUBSYSTEM_MAXVAL + 1; i++)
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

error_t	callbackStreamC::enqueue(zcallback::headerS *callback)
{
	error_t		ret;

	if (callback == __KNULL) { return ERROR_INVALID_ARG; };
	if (callback->subsystem > ZMESSAGE_SUBSYSTEM_MAXVAL) {
		return ERROR_INVALID_ARG_VAL;
	};

	/**	TODO:
	 * Think about this type of situation and determine whether or not it's
	 * safe to execute within the critical section with interrupts enabled
	 * on the local CPU. Most likely not though. Low priority as well
	 * since this is mostly a throughput optimization and not a
	 * functionality tweak.
	 **/
	pendingSubsystems.lock();

	ret = queues[callback->subsystem].addItem(callback);
	if (ret == ERROR_SUCCESS)
	{
		pendingSubsystems.set(callback->subsystem);
		/* Unblock the thread. This may be a normal thread, or a per-cpu
		 * thread. In the case of it being a normal thread, no extra
		 * work is required: just unblock() it.
		 *
		 * If it's a per-CPU thread, we need to unblock it /on the
		 * target CPU/.
		 **/
		if (parent->contextType == task::PER_CPU) {
			taskTrib.unblock(parent->parent.cpu);
		} else {
			taskTrib.unblock(parent->parent.thread);
		};
	};

	pendingSubsystems.unlock();

__kprintf(NOTICE"Just enqueued message. Ret is %d, subsystem bit %d.\n", ret, pendingSubsystems.testSingle(callback->subsystem));
	return ret;
}

