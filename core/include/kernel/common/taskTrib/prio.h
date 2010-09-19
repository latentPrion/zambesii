#ifndef _TASK_PRIORITIES_H
	#define _TASK_PRIORITIES_H

	#include <__kstdlib/__ktypes.h>

#define PRIOINDEX_REALTIME		(0)
#define PRIOINDEX_CRITICAL		(1)
#define PRIOINDEX_HIGH			(2)
#define PRIOINDEX_NORMAL		(3)
#define PRIOINDEX_LOW			(4)
#define PRIOINDEX_NEGLIGABLE		(5)

#define PRIOCLASS_REALTIME		(1)
#define PRIOCLASS_CRITICAL		(7)
#define PRIOCLASS_HIGH			(13)
#define PRIOCLASS_NORMAL		(19)
#define PRIOCLASS_LOW			(25)
#define PRIOCLASS_NEGLIGABLE		(31)


#define PRIO_NCLASSES			(6)

typedef sbit16		prio_t;

#endif

