#ifndef _TASK_LIST_NODE_H
	#define _TASK_LIST_NODE_H

	#include <kernel/common/task.h>

struct taskListNodeS
{
	taskS		*task;
	taskListNodeS	*prev, *next;
};	

#endif

