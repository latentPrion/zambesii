#ifndef _SINGLE_WAITER_QUEUE_H
	#define _SINGLE_WAITER_QUEUE_H

	#include <__kclasses/ptrDoubleList.h>
	#include <kernel/common/task.h>
	#include <kernel/common/taskTrib/taskTrib.h>

#define SWAITQ		"Single-WaiterQ: "

template <class T>
class singleWaiterQueueC
:
public pointerDoubleListC<T>
{
public:
	singleWaiterQueueC(taskC *task=__KNULL)
	:
	task(task)
	{}

	error_t initialize(taskC *task=__KNULL)
	{
		if (pointerDoubleListC<T>::initialize() != ERROR_SUCCESS) {
			return ERROR_UNKNOWN;
		};

		if (task == __KNULL && this->task == __KNULL) {
			return ERROR_CRITICAL;
		};

		this->task = task;
		return ERROR_SUCCESS;
	};

	~singleWaiterQueueC(void) {}

public:
	error_t addItem(T *item)
	{
		error_t		ret;
		ubit8		needToWake=0;

		if (pointerDoubleListC<T>::getNItems() == 0) {
			needToWake = 1;
		};

		ret = pointerDoubleListC<T>::addItem(item);
		if (ret != ERROR_SUCCESS) { return ret; };

		if (needToWake)
		{
__kprintf(NOTICE"Unblocking.\n");
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
	T *pop(void)
	{
		T	*ret;

		/**	NOTE:
		 * It may be possible for another CPU to simultaneously have
		 * added an item between after we called pop(), and before
		 * the call to block() is completed. Think of a way
		 * to guarantee that this will not cause any problems.
		 **/
		ret = pointerDoubleListC<T>::popFromHead();
		for (; ret == __KNULL;
			ret = pointerDoubleListC<T>::popFromHead())
		{
__kprintf(NOTICE"Going to sleep\n");
			taskTrib.block();
		};

		return ret;
	}

	taskC *getThread(void)
	{
		return task;
	};

	taskC *getTask(void)
	{
		return getThread();
	}

private:
	taskC		*task;
};

#endif

