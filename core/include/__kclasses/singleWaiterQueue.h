#ifndef _SINGLE_WAITER_QUEUE_H
	#define _SINGLE_WAITER_QUEUE_H

	#include <__kstdlib/__kflagManipulation.h>
	#include <__kclasses/ptrDoubleList.h>
	#include <kernel/common/waitLock.h>

#define SWAITQ		"Single-WaiterQ: "

#define SINGLEWAITERQ_POP_FLAGS_DONTBLOCK	(1<<0)

class Thread;

class SingleWaiterQueue
:
public PtrDblList<void>
{
public:
	SingleWaiterQueue(void)
	:
	thread(NULL)
	{}

	error_t initialize(void)
	{
		return PtrDblList<void>::initialize();
	};

	~SingleWaiterQueue(void) {}

public:
	error_t addItem(void *item);
	// PtrDblList::remove() is sufficient, needs no extending.
	// PtrDblList::getHead() is sufficient, needs no extending.
	// PtrDblList::getNItems() is sufficient, needs no extending.
	error_t pop(void **ret, uarch_t flags=0);
	error_t setWaitingThread(Thread *thread);

	Thread *getThread(void) { return thread; }

private:
	Thread		*thread;
	WaitLock	lock;
};

#endif

