
#include <__kclasses/singleWaiterQueue.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/task.h>
#include <kernel/common/taskTrib/taskTrib.h>


error_t singleWaiterQueueC::addItem(void *item)
{
	error_t		ret;
	ubit8		needToWake=0;

	if (task != __KNULL && task->runState != taskC::RUNNABLE
		&& task->runState != taskC::RUNNING)
	{
		needToWake = 1;
	};

	ret = pointerDoubleListC<void>::addItem(item);
	if (ret != ERROR_SUCCESS) { return ret; };

	if (needToWake && task != __KNULL)
	{
		ret = taskTrib.unblock(task);
		if (ret != ERROR_SUCCESS)
		{
			__kprintf(NOTICE SWAITQ"Failed to unblock "
				"task 0x%x.\n", task->id);

			pointerDoubleListC<void>::removeItem(item);
			return ret;
		};
	};

	return ERROR_SUCCESS;
}

void *singleWaiterQueueC::pop(uarch_t flags)
{
	void	*ret;

	/**	NOTE:
	 * It may be possible for another CPU to simultaneously have
	 * added an item between after we called pop(), and before
	 * the call to block() is completed. Think of a way
	 * to guarantee that this will not cause any problems.
	 **/
	ret = pointerDoubleListC<void>::popFromHead();
	for (;
		!__KFLAG_TEST(flags, SINGLEWAITERQ_POP_FLAGS_DONTBLOCK)
			&& ret == __KNULL;
		ret = pointerDoubleListC<void>::popFromHead())
	{
__kprintf(NOTICE"Going to sleep\n");
		taskTrib.block();
	};

	return ret;
}

error_t singleWaiterQueueC::setWaitingThread(taskC *task)
{
	if (task == __KNULL) { return ERROR_INVALID_ARG; };

	if (this->task != __KNULL
		&& this->task->parent->id != task->parent->id)
	{
		__kprintf(WARNING SWAITQ"Failed to allow task 0x%x to "
			"wait.\n",
			task->id);

		return ERROR_RESOURCE_BUSY;
	}

	this->task = task;
	return ERROR_SUCCESS;
}

