
#include <arch/arch.h>
#include <arch/paging.h>
#include <__kstdlib/__ktypes.h>
#include <kernel/common/task.h>


taskS	__korientationThread;
ubit8 __korientationStack[PAGING_BASE_SIZE * ARCH___KSTACK_NPAGES];

