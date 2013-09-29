#ifndef _SINGLE_WAITER_QUEUE_H
	#define _SINGLE_WAITER_QUEUE_H

	#include <__kstdlib/__kflagManipulation.h>
	#include <__kclasses/ptrDoubleList.h>
	#include <kernel/common/waitLock.h>

#define SWAITQ		"Single-WaiterQ: "

#define SINGLEWAITERQ_POP_FLAGS_DONTBLOCK	(1<<0)

class threadC;

class singleWaiterQueueC
:
public pointerDoubleListC<void>
{
public:
	singleWaiterQueueC(void)
	:
	thread(NULL)
	{}

	error_t initialize(void)
	{
		return pointerDoubleListC<void>::initialize();
	};

	~singleWaiterQueueC(void) {}

public:
	error_t addItem(void *item);
	// pointerDoubleListC::remove() is sufficient, needs no extending.
	// pointerDoubleListC::getHead() is sufficient, needs no extending.
	// pointerDoubleListC::getNItems() is sufficient, needs no extending.
	error_t pop(void **ret, uarch_t flags=0);
	error_t setWaitingThread(threadC *thread);

	threadC *getThread(void) { return thread; }

private:
	threadC		*thread;
	waitLockC	lock;
};

#endif

