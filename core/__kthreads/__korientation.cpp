
#include <chipset/memory.h>
#include <arch/paging.h>
#include <__kstdlib/__ktypes.h>
#include <kernel/common/thread.h>
#include <kernel/common/processTrib/processTrib.h>


threadC	__korientationThread(__KPROCESSID, processTrib.__kgetStream(), NULL);
ubit8	__korientationStack[PAGING_BASE_SIZE * CHIPSET_MEMORY___KSTACK_NPAGES];

