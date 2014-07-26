#ifndef _WAIT_QUEUE_H
	#define _WAIT_QUEUE_H

	#include <__kstdlib/__ktypes.h>

template <class T>
class waitQueueC
{
public:
	waitQueueC(void);
	error_t initialize(void);

	uarch_t getNItems(void);
	uarch_t getNWaiters(void);
};

#endif

