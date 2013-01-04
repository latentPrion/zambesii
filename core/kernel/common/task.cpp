
#include <chipset/memory.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/task.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/process.h>

taskC::taskC(processId_t taskId, processStreamC *parentProcess)
:
id(taskId), parent(parentProcess)
{
	stack0 = stack1 = __KNULL;

	context = __KNULL;
	flags = 0;

	schedPrio = &internalPrio;
	schedFlags = 0;
	schedState = UNSCHEDULED;
	currentCpu = __KNULL;

	nLocksHeld = 0;
#ifdef CONFIG_PER_TASK_TLB_CONTEXT
	tlbContext = __KNULL;
#endif
}

error_t taskC::initialize(void)
{
	stack0 = memoryTrib.__kmemoryStream.memAlloc(
			CHIPSET_MEMORY___KSTACK_NPAGES, MEMALLOC_NO_FAKEMAP);

	if (stack0 == __KNULL) { return ERROR_MEMORY_NOMEM; };

	// TODO: Implement memoryStreamC::stackAlloc().
	stack1 = parent->memoryStream->memAlloc(4, 0);
	if (stack1 == __KNULL) { return ERROR_MEMORY_NOMEM; };

	context = new taskContextS;
	if (context == __KNULL) { return ERROR_MEMORY_NOMEM; };

#ifdef CONFIG_PER_TASK_TLB_CONTEXT
	tlbContext = new tlbContextS;
	if (tlbContext == __KNULL) { return ERROR_MEMORY_NOMEM; };
#endif

	return ERROR_SUCCESS;
}

error_t taskC::initializeChild(taskC *child)
{
	error_t		ret;

	child->flags = flags;

	child->internalPrio = internalPrio;
	if (schedPrio != &internalPrio) {
		child->schedPrio = schedPrio;
	};

	child->schedPolicy = schedPolicy;
	child->schedOptions = schedOptions;
	// 'schedFlags' and 'schedState' are not inherited.

	// Copy affinity.
	ret = affinity::copyLocal(&child->localAffinity, &localAffinity);
	child->localAffinity.def.rsrc = NUMABANKID_INVALID;
	return ret;
}

