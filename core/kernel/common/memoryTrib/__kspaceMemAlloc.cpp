
#include <kernel/common/memoryTrib/__kspaceMemAlloc.h>
#include <kernel/common/memoryTrib/memoryTrib.h>

void *__kspaceMemAlloc(uarch_t nPages)
{
	return memoryTrib.__kspaceMemAlloc(nPages);
}

void __kspaceMemFree(void *vaddr, uarch_t nPages)
{
	memoryTrib.__kspaceMemFree(vaddr, nPages);
}

