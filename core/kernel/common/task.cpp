
#include <chipset/memory.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/task.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/process.h>


error_t taskC::initialize(void)
{
	stack0 = processTrib.__kprocess.memoryStream.memAlloc(
			CHIPSET_MEMORY___KSTACK_NPAGES, MEMALLOC_NO_FAKEMAP);

	if (stack0 == __KNULL) { return ERROR_MEMORY_NOMEM; };

	// TODO: Implement memoryStreamC::stackAlloc().
	stack1 = parent->memoryStream.memAlloc(4, 0);
	if (stack1 == __KNULL) { return ERROR_MEMORY_NOMEM; };

#ifdef CONFIG_PER_TASK_TLB_CONTEXT
	if (tlbContext.initialize() != ERROR_SUCCESS) { return ERROR_UNKNOWN; };
#endif

	return ERROR_SUCCESS;
}

static inline error_t resizeAndMergeBitmaps(bitmapC *dest, bitmapC *src)
{
	error_t		ret;

	ret = dest->resizeTo(src->getNBits());
	if (ret != ERROR_SUCCESS) { return ret; };

	dest->merge(src);
	return ERROR_SUCCESS;
}

error_t taskC::cloneStateIntoChild(taskC *child)
{
	child->flags = flags;

	child->internalPrio = internalPrio;
	if (schedPrio != &internalPrio) {
		child->schedPrio = schedPrio;
	};

	child->schedPolicy = schedPolicy;
	child->schedOptions = schedOptions;
	// 'schedFlags' and 'schedState' are not inherited.

	// Copy affinity.
	child->defaultMemoryBank.rsrc = NUMABANKID_INVALID;
	return resizeAndMergeBitmaps(&child->cpuAffinity, &cpuAffinity);
}

