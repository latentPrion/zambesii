#ifndef _TASK_LIST_NODE_H
	#define _TASK_LIST_NODE_H

	#include <kernel/common/task.h>

struct taskQNodeS
{
	taskS		*task;
	taskQNodeS	*prev, *next;
};	

#endif

