
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

error_t memoryTribPmm::contiguousGetFrames(uarch_t nFrames, paddr_t *paddr)
{
	return memoryTrib.contiguousGetFrames(nFrames, paddr);
}

status_t memoryTribPmm::fragmentedGetFrames(uarch_t nFrames, paddr_t *paddr)
{
	return memoryTrib.fragmentedGetFrames(nFrames, paddr);
}

#if __SCALING__ >= SCALING_CC_NUMA
status_t memoryTribPmm::configuredGetFrames(
	bitmapC *cpuAffinity,
	sharedResourceGroupC<multipleReaderLockC, numaBankId_t>
		*defaultMemoryBank,
	uarch_t nFrames, paddr_t *paddr
	)
{
	return memoryTrib.configuredGetFrames(
		cpuAffinity, defaultMemoryBank, nFrames, paddr);
}
#endif

void memoryTribPmm::releaseFrames(paddr_t paddr, uarch_t nFrames)
{
	memoryTrib.releaseFrames(paddr, nFrames);
}

