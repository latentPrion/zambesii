
#include <__kclasses/debugPipe.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/memoryTrib/rawMemAlloc.h>
#include <kernel/common/memoryTrib/pmmBridge.h>
#include <kernel/common/floodplainn/dma.h>

void *rawMemAlloc(uarch_t nPages, uarch_t flags)
{
	return memoryTrib.rawMemAlloc(nPages, flags);
}

void rawMemFree(void *vaddr, uarch_t nPages)
{
	memoryTrib.rawMemFree(vaddr, nPages);
}

status_t memoryTribPmm::fragmentedGetFrames(uarch_t nFrames, paddr_t *paddr)
{
	return memoryTrib.fragmentedGetFrames(nFrames, paddr);
}

status_t memoryTribPmm::constrainedGetFrames(
	fplainn::dma::constraints::Compiler *comCon,
	uarch_t nFrames,
	fplainn::dma::ScatterGatherList *retlist,
	uarch_t flags
	)
{
	return memoryTrib.constrainedGetFrames(comCon, nFrames, retlist, flags);
}

void memoryTribPmm::releaseFrames(paddr_t paddr, uarch_t nFrames)
{
	memoryTrib.releaseFrames(paddr, nFrames);
}

