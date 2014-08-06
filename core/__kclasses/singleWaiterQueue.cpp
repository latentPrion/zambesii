
#include <__kclasses/singleWaiterQueue.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/thread.h>
#include <kernel/common/taskTrib/taskTrib.h>


error_t SingleWaiterQueue::addItem(void *item)
{
	error_t		ret;

	// Prevent lost wakeups from race conditions.
	lock.acquire();

	if (thread == NULL)
	{
		lock.release();
		return ERROR_UNINITIALIZED;
	};

	ret = PtrDblList<void>::addItem(item);
	if (ret != ERROR_SUCCESS)
	{
		lock.release();
		return ret;
	};

	ubit32		nItems;

	nItems = PtrDblList<void>::getNItems();
	if (nItems == 1)
	{
		ret = taskTrib.unblock(thread);
		if (ret != ERROR_SUCCESS)
		{
			PtrDblList<void>::removeItem(item);

			lock.release();

			printf(NOTICE SWAITQ"Failed to unblock task 0x%x.\n",
				thread->getFullId());

			return ret;
		};

		lock.release();
		return ret;
	};

	lock.release();
	return ERROR_SUCCESS;
}

error_t SingleWaiterQueue::pop(void **item, uarch_t flags)
{
	for (;FOREVER;)
	{
		// Prevent lost wakeups from race conditions.
		lock.acquire();

		*item = PtrDblList<void>::popFromHead();
		if (*item != NULL)
		{
			lock.release();
			return ERROR_SUCCESS;
		};

		if (FLAG_TEST(flags, SINGLEWAITERQ_POP_FLAGS_DONTBLOCK))
		{
			lock.release();
			return ERROR_WOULD_BLOCK;
		};

		Lock::sOperationDescriptor	unlockDescriptor(
			&lock,
			Lock::sOperationDescriptor::WAIT);

		taskTrib.block(&unlockDescriptor);
	};
}

error_t SingleWaiterQueue::setWaitingThread(Thread *newThread)
{
	if (newThread == NULL) { return ERROR_INVALID_ARG; };

	// Only allow threads from the currently owning process to wait.
	if (thread != NULL
		&& thread->parent->id != newThread->parent->id)
	{
		printf(WARNING SWAITQ"Failed to allow task 0x%x to "
			"wait.\n",
			newThread->getFullId());

		return ERROR_RESOURCE_BUSY;
	}

	thread = newThread;
	return ERROR_SUCCESS;
}

