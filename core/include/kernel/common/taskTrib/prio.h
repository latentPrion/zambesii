#ifndef _TASK_PRIORITIES_H
	#define _TASK_PRIORITIES_H

	#include <__kstdlib/__ktypes.h>

/**	EXPLANATION:
 * The Zambesii scheduler has a total of 20 priorities to which a task can
 * belong. As of this writing, the kernel supports two scheduling policies:
 *	1. Priority-enabled round-robin,
 *	2. Priority-enabled real-time.
 *
 * A real-time task is placed in the real-time queue on its CPU. A CPU always
 * checks its real-time queue before checking its round-robin queue, so real-
 * time threads automatically, and invariably will be pulled before round-robin
 * policy threads.
 *
 * Within each queue, whether RR or realtime, the threads are ordered in lists
 * by priority. Each priority number has its own queue of threads which are
 * of that priority.
 *
 * There are a total of (at the time of writing) 20 priorities.
 *
 * Apart from the policy and priority of a thread, threads have properties which
 * affect the way they are queued, even relative to other threads of the same
 * priority.
 *	SCHEDATTR_STICKY: when the thread is returned to its queue, it is
 *		placed at the front of all threads of the same priority.
 *		This means that as long as this thread is runnable, and no tasks
 *		of higher priority exist, this thread will always run.
 *
 *		This flag is off by default. Threads will be added at the back
 *		of their peer priority thread list by default.
 **/

#define SCHEDPRIO_MAX_NPRIOS		20
#define SCHEDPRIO_DEFAULT		10

#define SCHEDOPTS_STICKY		(1<<0)

#define QUANTUMCLASS_NCLASSES		5

typedef ubit16		prio_t;

#endif

