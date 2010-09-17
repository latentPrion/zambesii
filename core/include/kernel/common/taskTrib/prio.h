#ifndef _TASK_PRIORITIES_H
	#define _TASK_PRIORITIES_H

	#include <__kstdlib/__ktypes.h>

#define PRIOINDEX_REALTIME		(0)
#define PRIOINDEX_IMPORTANT		(1)
#define PRIOINDEX_FAST			(2)
#define PRIOINDEX_NORMAL		(3)
#define PRIOINDEX_DELAYABLE		(4)
#define PRIOINDEX_NEGLIGABLE		(5)

#define PRIOCLASS_REALTIME		(0)
#define PRIOCLASS_IMPORTANT		(1)
#define PRIOCLASS_FAST			(2)
#define PRIOCLASS_NORMAL		(3)
#define PRIOCLASS_DELAYABLE		(4)
#define PRIOCLASS_NEGLIGABLE		(5)


#define PRIO_NCLASSES			(6)

typedef sbit16		prio_t;

#endif

