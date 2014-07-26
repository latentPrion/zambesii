#ifndef _MEMORY_TRIB_RAW_MEM_ALLOC_H
	#define _MEMORY_TRIB_RAW_MEM_ALLOC_H

	#include <__kstdlib/__ktypes.h>

/**	EXPLANATION:
 * In order to lessen the number of circular dependencies in the kernel, these
 * wrapper functions around MemoryTrib::rawMemAlloc() and
 * MemoryTrib::rawMemFree() have been implemented. Their sole purpose is to
 * allow circumnavigation of the need to include memoryTrib.h where it causes
 * an #include circular chaotic hell.
 **/

void *rawMemAlloc(uarch_t nPages, uarch_t flags);
void rawMemFree(void *vaddr, uarch_t nPages);

#endif

