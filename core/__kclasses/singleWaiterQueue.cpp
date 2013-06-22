
#include <__kclasses/singleWaiterQueue.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/thread.h>
#include <kernel/common/taskTrib/taskTrib.h>


error_t singleWaiterQueueC::addItem(void *item)
{
	error_t		ret;

	// Prevent lost wakeups from race conditions.
	lock.acquire();

	if (thread == __KNULL)
	{
		lock.release();
		return ERROR_UNINITIALIZED;
	};

	ret = pointerDoubleListC<void>::addItem(item);
	if (ret != ERROR_SUCCESS)
	{
		lock.release();
		return ret;
	};

	ubit32		nItems;

	nItems = pointerDoubleListC<void>::getNItems();
	if (nItems == 1)
	{
		ret = taskTrib.unblock(thread);
		if (ret != ERROR_SUCCESS)
		{
			pointerDoubleListC<void>::removeItem(item);

			lock.release();

			__kprintf(NOTICE SWAITQ"Failed to unblock task 0x%x.\n",
				thread->getFullId());

			return ret;
		};

		lock.release();
		return ret;		
	};

	lock.release();
	return ERROR_SUCCESS;
}

error_t singleWaiterQueueC::pop(void **item, uarch_t flags)
{
	for (;;)
	{
		// Prevent lost wakeups from race conditions.
		lock.acquire();

		*item = pointerDoubleListC<void>::popFromHead();
		if (*item != __KNULL)
		{
			lock.release();
			return ERROR_SUCCESS;
		};

		if (__KFLAG_TEST(flags, SINGLEWAITERQ_POP_FLAGS_DONTBLOCK))
		{
			lock.release();
			return ERROR_WOULD_BLOCK;
		};

		taskTrib.block(&lock, TASKTRIB_BLOCK_LOCKTYPE_WAIT);
	};
}

error_t singleWaiterQueueC::setWaitingThread(threadC *newThread)
{
	if (newThread == __KNULL) { return ERROR_INVALID_ARG; };

	// Only allow threads from the currently owning process to wait.
	if (thread != __KNULL
		&& thread->parent->id != newThread->parent->id)
	{
		__kprintf(WARNING SWAITQ"Failed to allow task 0x%x to "
			"wait.\n",
			newThread->getFullId());

		return ERROR_RESOURCE_BUSY;
	}

	thread = newThread;
	return ERROR_SUCCESS;
}

