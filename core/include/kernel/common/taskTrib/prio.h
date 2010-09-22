#ifndef _TASK_PRIORITIES_H
	#define _TASK_PRIORITIES_H

	#include <__kstdlib/__ktypes.h>

/**	EXPLANATION:
 * Zambezii's scheduler has two main type of scheduling policies for tasks:
 *	1. "Prio" scheduling, or realtime.
 *	2. "Flow" scheduling, or normal.
 *
 * Scheduling is done via per-CPU queues. Each queue has a prioQ, a flowQ and
 * a dormantQ. Realtime tasks are added to the prioQ, non-real time tasks are
 * added to the flowQ, sleeping tasks are added to the dormantQ.
 *
 * The prioQ is always checked for tasks first. Apart from the type of policy
 * used to schedule the process, every process has a set of additional options,
 * such as how it should be treated within the queue (see SCHEDTYPE_*).
 *
 * Every thread has a hard quantum value. "Hard" quantums are the exact number
 * of timer ticks that this task will execute for at most; that is, the task's
 * quantum. The user APIs set quantums in "soft" values, which are an abstract
 * scale of quantums which range from 0-32, and the kernel translates soft
 * quantums into hard quantums when actually setting quantum values.
 *
 * The kernel will have an API which will allow the user to, from userspace,
 * exert a certain amount of control over the scheduler, by creating
 * quantum classes. A quantum class is a quantum value. Multiple processes may
 * all point to the same quantum class (and thus have the same quantum value).
 *
 * All driver tasks unchangeably share the quantum class QUANTUMCLASS_DRIVER.
 * All non-driver tasks which have not explicitly been set to some quantum value
 * share the class QUANTUMCLASS_NORMAL. By changing the value assigned to the
 * quantum class, one can change the quantums of all tasks which share that
 * quantum class.
 *
 * Along with the two default, unremovable classes the kernel generates at boot,
 * the user can create new classes, and assign any number of processes to that
 * class. Of course, the kernel constrains driver processes to only point to
 * the QUANTUMCLASS_DRIVER class. But all non-driver tasks can be set to any
 * quantum class.
 **/

// Set to enable real-time scheduling for task. Clear to make normal task.
#define SCHEDFLAG_PRIO			(1<<0)

// When timeslice is up, place at back of queue.
#define SCHEDTYPE_FIFO			(0x1)
// When timeslice is up, continue executing.
#define SCHEDTYPE_STICKY		(0x2)
// Only execute if no other tasks in queue.
#define SCHEDTYPE_LASTRESORT		(0x3)


#define QUANTUMCLASS_NCLASSES		(5)

#define QUANTUMCLASS_NORMAL		(0)
#define QUANTUMCLASS_DRIVER		(1)

#define QUANTUMCLASS_NORMAL_INITVAL	(SOFTQUANTUM_NORMAL)
#define QUANTUMCLASS_DRIVER_INITVAL	(SOFTQUANTUM_NORMAL)

#define SOFTQUANTUM_IGNORE		0
#define SOFTQUANTUM_LOW			1
#define SOFTQUANTUM_NORMAL		11
#define SOFTQUANTUM_HIGH		21
#define SOFTQUANTUM_CRITICAL		31

typedef sbit16		prio_t;

#endif

