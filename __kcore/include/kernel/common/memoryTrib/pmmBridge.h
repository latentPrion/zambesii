#ifndef _PMM_BRIDGE_H
	#define _PMM_BRIDGE_H

	#include <scaling.h>
	#include <arch/paddr_t.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/bitmap.h>
	#include <kernel/common/floodplainn/zudi.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/multipleReaderLock.h>

namespace fplainn
{
namespace dma
{
	class Constraints;
	class ScatterGatherList;

namespace constraints
{
	class Compiler;
}
}
}

namespace memoryTribPmm
{
	status_t fragmentedGetFrames(uarch_t nFrames, paddr_t *paddr);
	status_t constrainedGetFrames(
		fplainn::dma::constraints::Compiler *comCon,
		uarch_t nFrames,
		fplainn::dma::ScatterGatherList *retlist,
		uarch_t flags=0);

	void releaseFrames(paddr_t basePaddr, uarch_t nFrames);
}

#endif

