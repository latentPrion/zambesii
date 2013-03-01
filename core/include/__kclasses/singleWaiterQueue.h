#ifndef _SINGLE_WAITER_QUEUE_H
	#define _SINGLE_WAITER_QUEUE_H

	#include <__kstdlib/__kflagManipulation.h>
	#include <__kclasses/ptrDoubleList.h>
	#include <kernel/common/taskTrib/taskTrib.h>

#define SWAITQ		"Single-WaiterQ: "

#define SINGLEWAITERQ_POP_FLAGS_DONTBLOCK	(1<<0)
class taskC;

template <class T>
class singleWaiterQueueC
:
public pointerDoubleListC<T>
{
public:
	singleWaiterQueueC(void)
	:
	task(__KNULL)
	{}

	error_t initialize(void)
	{
		return pointerDoubleListC<T>::initialize();
	};

	~singleWaiterQueueC(void) {}

public:
	error_t addItem(T *item)
	{
		error_t		ret;
		ubit8		needToWake=0;

		if (task != __KNULL && task->runState != taskC::RUNNABLE
			&& task->runState != taskC::RUNNING)
		{
			needToWake = 1;
		};

		ret = pointerDoubleListC<T>::addItem(item);
		if (ret != ERROR_SUCCESS) { return ret; };

		if (needToWake && task != __KNULL)
		{
			ret = taskTrib.unblock(task);
			if (ret != ERROR_SUCCESS)
			{
				__kprintf(NOTICE SWAITQ"Failed to unblock "
					"task 0x%x.\n", task->id);

				pointerDoubleListC<T>::removeItem(item);
				return ret;
			};
		};

		return ERROR_SUCCESS;
	};

	// pointerDoubleListC::remove() is sufficient, needs no extending.
	// pointerDoubleListC::getHead() is sufficient, needs no extending.
	// pointerDoubleListC::getNItems() is sufficient, needs no extending.
	T *pop(uarch_t flags=0)
	{
		T	*ret;

		/**	NOTE:
		 * It may be possible for another CPU to simultaneously have
		 * added an item between after we called pop(), and before
		 * the call to block() is completed. Think of a way
		 * to guarantee that this will not cause any problems.
		 **/
		ret = pointerDoubleListC<T>::popFromHead();
		for (;
			!__KFLAG_TEST(flags, SINGLEWAITERQ_POP_FLAGS_DONTBLOCK)
				&& ret == __KNULL;
			ret = pointerDoubleListC<T>::popFromHead())
		{
__kprintf(NOTICE"Going to sleep\n");
			taskTrib.block();
		};

		return ret;
	}

	error_t setWaitingThread(taskC *task)
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

	error_t setWaitingTask(taskC *task)
	{
		return setWaitingThread(task);
	}

	taskC *getThread(void)
	{
		return task;
	}

	taskC *getTask(void)
	{
		return getThread();
	}

private:
	taskC		*task;
};

#endif

