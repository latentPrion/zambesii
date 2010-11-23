#ifndef _TASK_STREAM_H
	#define _TASK_STREAM_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/slamCache.h>
	#include <kernel/common/stream.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>
	#include <kernel/common/taskTrib/taskQNode.h>

class taskStreamC
:
public streamC
{
public:
	taskStreamC(void)
	:
	taskQNodeCache(sizeof(taskQNodeS))
	{};

public:
	ubit32 getLoad(void) { return load; };
	ubit32 getCapacity(void) { return capacity; };

	error_t addTask(taskS *task);

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

public:
	ubit32		load;
	ubit32		capacity;

private:
	// Each CPU has its own allocator for taskQNodeS blocks.
	slamCacheC	taskQNodeCache;
	sharedResourceGroupC<waitLockC, taskQNodeS *>	prioQ, flowQ, dormantQ;
};

#endif

