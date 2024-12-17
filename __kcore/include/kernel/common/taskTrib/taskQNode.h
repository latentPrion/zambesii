#ifndef _TASK_LIST_NODE_H
	#define _TASK_LIST_NODE_H

	#include <kernel/common/thread.h>

struct sThreadQueueNode
{
	Thread		*thread;
	sThreadQueueNode	*prev, *next;
};

#endif

