
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/memoryTrib/rawMemAlloc.h>

void *rawMemAlloc(uarch_t nPages)
{
	return memoryTrib.rawMemAlloc(nPages);
}

void rawMemFree(void *vaddr, uarch_t nPages)
{
	memoryTrib.rawMemFree(vaddr, nPages);
}

