#ifndef _TASK_STREAM_H
	#define _TASK_STREAM_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/slamCache.h>
	#include <kernel/common/stream.h>

class taskStreamC
:
public streamC
{
public:
	// Used by the taskTrib to add/remove tasks from a CPU.
	error_t addToFlowQ(void);
	taskS *rmFromFlowQ(void);

	error_t addToDormantQ(void);
	taskS *rmFromDormantQ(void);

	error_t addToPrioQ(void);
	taskS *rmFromPrioQ(void);

public:
	// Used by CPU to manipulate tasks in Qs.
	taskS *pullFlowQ(void);
	taskS *pullPrioQ(void);

private:
	// Each CPU has its own allocator for taskQNodeS blocks.
	slamCacheC	taskQNodeCache;
	taskQNodeS	*prioQ, *flowQ, *dormantQ;
};

#endif

