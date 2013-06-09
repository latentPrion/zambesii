
#include <__kclasses/singleWaiterQueue.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/task.h>
#include <kernel/common/taskTrib/taskTrib.h>


error_t singleWaiterQueueC::addItem(void *item)
{
	error_t		ret;

	// Prevent lost wakeups from race conditions.
	lock.acquire();

	if (task == __KNULL)
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

	// The "3" is to reduce chances of lost wakeups.
	if (pointerDoubleListC<void>::getNItems() < 3)
	{
		ret = taskTrib.unblock(task);
		if (ret != ERROR_SUCCESS)
		{
			pointerDoubleListC<void>::removeItem(item);

			lock.release();

			__kprintf(NOTICE SWAITQ"Failed to unblock task 0x%x.\n",
				task->getFullId());

			return ret;
		};
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

error_t singleWaiterQueueC::setWaitingThread(taskC *task)
{
	if (task == __KNULL) { return ERROR_INVALID_ARG; };

	// Only allow threads from the currently owning process to wait.
	if (this->task != __KNULL
		&& this->task->parent->id != task->parent->id)
	{
		__kprintf(WARNING SWAITQ"Failed to allow task 0x%x to "
			"wait.\n",
			task->getFullId());

		return ERROR_RESOURCE_BUSY;
	}

	this->task = task;
	return ERROR_SUCCESS;
}

