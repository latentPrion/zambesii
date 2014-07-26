
#include <__kclasses/debugPipe.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/memoryTrib/rawMemAlloc.h>
#include <kernel/common/memoryTrib/pmmBridge.h>

void *rawMemAlloc(uarch_t nPages, uarch_t flags)
{
	return memoryTrib.rawMemAlloc(nPages, flags);
}

void rawMemFree(void *vaddr, uarch_t nPages)
{
	memoryTrib.rawMemFree(vaddr, nPages);
}

error_t memoryTribPmm::contiguousGetFrames(uarch_t, paddr_t *)
{
	// return memoryTrib.contiguousGetFrames(nFrames, paddr);
	UNIMPLEMENTED("MemoryTrib::contiguousGetFrames()");
	return ERROR_UNIMPLEMENTED;
}

status_t memoryTribPmm::fragmentedGetFrames(uarch_t nFrames, paddr_t *paddr)
{
	return memoryTrib.fragmentedGetFrames(nFrames, paddr);
}

void memoryTribPmm::releaseFrames(paddr_t paddr, uarch_t nFrames)
{
	memoryTrib.releaseFrames(paddr, nFrames);
}

