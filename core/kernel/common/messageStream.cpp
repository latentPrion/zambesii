
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string.h>
#include <kernel/common/messageStream.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/processTrib/processTrib.h>

error_t messageStreamC::enqueueOnThread(
	processId_t targetStreamId, zmessage::headerS *header
	)
{
	messageStreamC	*targetStream;

	if (__KFLAG_TEST(header->flags, ZMESSAGE_FLAGS_CPU_TARGET))
	{
		cpuStreamC		*cs;

		/* Dealing with an asynchronous response to an API call from a
		 * per-CPU thread.
		 **/
		cs = cpuTrib.getStream((cpu_t)targetStreamId);
		if (cs == __KNULL) { return ERROR_INVALID_RESOURCE_NAME; };

		targetStream = &cs->getTaskContext()->messageStream;
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

		targetStream = &targetThread->getTaskContext()->messageStream;
	};

	return targetStream->enqueue(header->subsystem, header);
}

error_t messageStreamC::pullFrom(
	ubit16 subsystemQueue, zmessage::iteratorS *callback,
	ubit32 flags
	)
{
	zmessage::headerS	*tmp;

	if (callback == __KNULL) { return ERROR_INVALID_ARG; };
	if (subsystemQueue > ZMESSAGE_SUBSYSTEM_MAXVAL)
		{ return ERROR_INVALID_ARG_VAL; };

	for (;;)
	{
		pendingSubsystems.lock();

		if (pendingSubsystems.test(subsystemQueue))
		{
			tmp = queues[subsystemQueue].popFromHead();
			if (queues[subsystemQueue].getNItems() == 0) {
				pendingSubsystems.unset(subsystemQueue);
			};

			pendingSubsystems.unlock();

			memcpy(callback, tmp, tmp->size);
			delete tmp;
			return ERROR_SUCCESS;
		};

		if (__KFLAG_TEST(flags, ZCALLBACK_PULL_FLAGS_DONT_BLOCK))
		{
			pendingSubsystems.unlock();
			return ERROR_WOULD_BLOCK;
		};

		lockC::operationDescriptorS	unlockDescriptor(
			&pendingSubsystems.bmp.lock,
			lockC::operationDescriptorS::WAIT);

		taskTrib.block(&unlockDescriptor);
	};
}

error_t messageStreamC::pull(
	zmessage::iteratorS *callback, ubit32 flags
	)
{
	zmessage::headerS	*tmp;

	if (callback == __KNULL) { return ERROR_INVALID_ARG; };

	for (;;)
	{
		pendingSubsystems.lock();

		for (ubit16 i=0; i<ZMESSAGE_SUBSYSTEM_MAXVAL + 1; i++)
		{
			if (pendingSubsystems.test(i))
			{
				tmp = queues[i].popFromHead();
				if (queues[i].getNItems() == 0) {
					pendingSubsystems.unset(i);
				};

				pendingSubsystems.unlock();

				memcpy(callback, tmp, tmp->size);
				return ERROR_SUCCESS;
			};
		};

		if (__KFLAG_TEST(flags, ZCALLBACK_PULL_FLAGS_DONT_BLOCK))
		{
			pendingSubsystems.unlock();
			return ERROR_WOULD_BLOCK;
		};

		lockC::operationDescriptorS	unlockDescriptor(
			&pendingSubsystems.bmp.lock,
			lockC::operationDescriptorS::WAIT);

		taskTrib.block(&unlockDescriptor);
	};
}

error_t	messageStreamC::enqueue(ubit16 queueId, zmessage::headerS *callback)
{
	error_t		ret;

	if (callback == __KNULL) { return ERROR_INVALID_ARG; };
	if (queueId > ZMESSAGE_SUBSYSTEM_MAXVAL) {
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

	ret = queues[queueId].addItem(callback);
	if (ret == ERROR_SUCCESS)
	{
		pendingSubsystems.set(queueId);
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
	return ret;
}

