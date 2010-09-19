#ifndef _TASK_PRIORITIES_H
	#define _TASK_PRIORITIES_H

	#include <__kstdlib/__ktypes.h>

/**	EXPLANATION:
 * The kernel uses 6 priority classes, where REALTIME has its own queue in
 * each CPU. The Queue for realtime tasks is checked first.
 *
 * Priorities range from 0-126, where 0 is a task which will be ignored by all
 * CPUs. Priorities 1-84 are for normal processes, and priorities 85-126 are
 * real time.
 *
 * Each task has a "hard" prio which is the literal number of ticks for which
 * it will run. "Soft" prio is a number from 1-126 which can be used to
 * uniformly express this priority independently of the chipset and arch.
 *
 * Priority classes allow the user to manipulate process priorities in groups
 * based on their priority class. The user can even create priority classes of
 * his or her own, and assign processes to that priority.
 *
 * So the user can customize his scheduler from userspace. So imagine a user
 * defining a priority class called "games", and defining it to soft priority
 * 91. Then, when he starts up a game, he tells the scheduler: run this process
 * on prio class "game". He can start N processes and tell the scheduler to use
 * that prio class.
 *
 * The kernel has several preset prio classes as well, which are:
 *	PRIOCLASS_NORMAL: Set for all new processes unless instructed otherwise.
 *	PRIOCLASS_DRIVER: Set for all driver processes.
 *		Driver processes cannot be re-assigned to new prio classes.
 *
 * The preset classes are set aside in their own array. Custom user prio
 * classes are stored in a dynamic array. When a user creates a new prio class
 * its index in the array is returned as an identifier.
 *
 * To delete a prio class, the kernel must be restarted.
 *
 * The kernel also brackets off soft priorities into low, normal, high,
 * critical and realtime values.
 *	0:	Task will be ignored by CPU.
 *	1-21:	Low priority task.
 *	22-43:	Normal priority task.
 *	44-63:	High priority task.
 *	64-84:	Critical priority task.
 *	85-126:	Real-time priority task.
 *
 * It is important to note that the kernel does not store priorities as soft
 * priorities. All priorities in the kernel are hard values. Soft prio values
 * are only used in API calls.
 **/
#define PRIOCLASS_NCLASSES		(5)

#define PRIOCLASS_INDEX_NORMAL		(0)
#define PRIOCLASS_INDEX_DRIVER		(1)

#define PRIOCLASS_NORMAL_INITVAL	(22)
#define PRIOCLASS_DRIVER_INITVAL	(43)

#define SOFTPRIO_IGNORE			0
#define SOFTPRIO_LOW			1
#define SOFTPRIO_NORMAL			22
#define SOFTPRIO_HIGH			44
#define SOFTPRIO_CRITICAL		64
#define SOFTPRIO_REALTIME		85


typedef sbit16		prio_t;

#endif

