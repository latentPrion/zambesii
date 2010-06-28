#ifndef _MEMORY_TRIB____KSPACE_MEM_ALLOC_H
	#define _MEMORY_TRIB____KSPACE_MEM_ALLOC_H

	#include <__kstdlib/__ktypes.h>

void *__kspaceMemAlloc(uarch_t nPages);
void __kspaceMemFree(void *vaddr, uarch_t nPages);

#endif

