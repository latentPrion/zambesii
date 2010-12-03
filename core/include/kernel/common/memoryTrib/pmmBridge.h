#ifndef _PMM_BRIDGE_H
	#define _PMM_BRIDGE_H

	#include <scaling.h>
	#include <arch/paddr_t.h>
	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/machineAffinity.h>

namespace memoryTribPmm
{
	error_t contiguousGetFrames(uarch_t nFrames, paddr_t *paddr);
	status_t fragmentedGetFrames(uarch_t nFrames, paddr_t *paddr);
#if __SCALING__ >= SCALING_CC_NUMA
	status_t configuredGetFrames(
		localAffinityS *aff, uarch_t nFrames, paddr_t *paddr);
#endif

	void releaseFrames(paddr_t basePaddr, uarch_t nFrames);
}

#endif

