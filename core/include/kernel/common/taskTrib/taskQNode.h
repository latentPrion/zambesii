#ifndef _TASK_LIST_NODE_H
	#define _TASK_LIST_NODE_H

	#include <kernel/common/thread.h>

struct taskQNodeS
{
	taskC		*task;
	taskQNodeS	*prev, *next;
};	

#endif

