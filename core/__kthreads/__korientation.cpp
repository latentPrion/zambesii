
#include <chipset/memory.h>
#include <arch/paging.h>
#include <__kstdlib/__ktypes.h>
#include <kernel/common/task.h>
#include <kernel/common/processTrib/processTrib.h>


taskC	__korientationThread(
	0x0, &processTrib.__kprocess,
	taskC::ROUND_ROBIN,
	PRIOCLASS_DEFAULT,
	TASK_FLAGS_CUSTPRIO);

ubit8	__korientationStack[PAGING_BASE_SIZE * CHIPSET_MEMORY___KSTACK_NPAGES];

