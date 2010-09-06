
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/memoryTrib/rawMemAlloc.h>

void *rawMemAlloc(uarch_t nPages, uarch_t flags)
{
	return memoryTrib.rawMemAlloc(nPages, flags);
}

void rawMemFree(void *vaddr, uarch_t nPages)
{
	memoryTrib.rawMemFree(vaddr, nPages);
}

