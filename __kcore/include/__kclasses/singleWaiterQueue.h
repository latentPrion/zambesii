#ifndef _SINGLE_WAITER_QUEUE_H
	#define _SINGLE_WAITER_QUEUE_H

	#include <__kstdlib/__kflagManipulation.h>
	#include <__kclasses/heapDoubleList.h>
	#include <kernel/common/waitLock.h>

#define SWAITQ		"Single-WaiterQ: "

#define SINGLEWAITERQ_POP_FLAGS_DONTBLOCK	(1<<0)

class Thread;

class SingleWaiterQueue
:
public HeapDoubleList<void>
{
public:
	SingleWaiterQueue(void)
	:
	thread(NULL)
	{}

	error_t initialize(void)
	{
		return HeapDoubleList<void>::initialize();
	};

	~SingleWaiterQueue(void) {}

	void dump(void)
	{
		printf(NOTICE SWAITQ"%p: thread %p.\n", this, thread);
		HeapDoubleList<void>::dump();
	}

public:
	error_t addItem(void *item);
	// HeapDoubleList::remove() is sufficient, needs no extending.
	// HeapDoubleList::getHead() is sufficient, needs no extending.
	// HeapDoubleList::getNItems() is sufficient, needs no extending.
	error_t pop(void **ret, uarch_t flags=0);
	error_t setWaitingThread(Thread *thread);

	Thread *getThread(void) { return thread; }

private:
	Thread		*thread;
	WaitLock	lock;
};

#endif

