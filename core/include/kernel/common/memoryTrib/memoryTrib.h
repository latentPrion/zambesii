#ifndef _MEMORY_TRIB_H
	#define _MEMORY_TRIB_H

	#include <scaling.h>
	#include <arch/paddr_t.h>
	#include <arch/paging.h>
	#include <chipset/regionMap.h>
	#include <chipset/zkcm/numaMap.h>
	#include <chipset/zkcm/memoryConfig.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/memBmp.h>
	#include <__kclasses/bitmap.h>
	#include <__kclasses/pageTableCache.h>
	#include <__kclasses/hardwareIdList.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/memoryRegion.h>
	#include <kernel/common/memoryTrib/memoryStream.h>
	#include <kernel/common/machineAffinity.h>

#define MEMTRIB		"Memory Trib: "

class memoryTribC
:
public tributaryC
{
public:
	memoryTribC(pagingLevel0S *level0Accessor, paddr_t level0Paddr);

	error_t initialize(
		void *swampStart, uarch_t swampSize,
		vSwampC::holeMapS *holeMap);

	error_t __kstreamInit(
		void *swampStart, uarch_t swampSize,
		vSwampC::holeMapS *holeMap);

	error_t __kspaceInit(void);
	error_t pmemInit(void);
	error_t memRegionInit(void);

public:
	void *rawMemAlloc(uarch_t nPages, uarch_t flags);
	void rawMemFree(void *vaddr, uarch_t nPages);

	error_t pageTablePop(paddr_t *paddr);
	void pageTablePush(paddr_t paddr);

	numaMemoryBankC *getBank(numaBankId_t bankId);
	error_t createBank(numaBankId_t bankId);
	void destroyBank(numaBankId_t bankId);

	/* NOTE:
	 * These next few functions are an exposure of the underlying
	 * numaMemoryBankC interfaces on each bank. The kernel can efficiently
	 * tell how best to serve an allocation most of the time. Therefore
	 * developers are encouraged to use these top-level functions and let
	 * the implementation logic behind the numaTrib decide what's best.
	 *
	 * Otherwise you end up with NUMA bank specifics in the kernel.
	 **/
	// Physical memory management functions.
#if __SCALING__ >= SCALING_CC_NUMA
	status_t configuredGetFrames(
		localAffinityS *aff, uarch_t nFrames, paddr_t *ret);
#endif
	status_t fragmentedGetFrames(uarch_t nFrames, paddr_t *ret);
	error_t contiguousGetFrames(uarch_t nFrames, paddr_t *ret);
	void releaseFrames(paddr_t paddr, uarch_t nFrames);

	void mapRangeUsed(paddr_t baseAddr, uarch_t nFrames);
	void mapRangeUnused(paddr_t baseAddr, uarch_t nFrames);

private:
	void init2_spawnNumaStreams(zkcmNumaMapS *map);
	void init2_generateNumaMemoryRanges(
		zkcmNumaMapS *map, sarch_t *__kspaceBool);

	void init2_generateShbankFromNumaMap(
		zkcmMemConfigS *cfg, zkcmNumaMapS *map, sarch_t *__kspaceBool);


public:
	memoryStreamC		__kmemoryStream;
	bitmapC			onlineBanks;

private:
	memoryRegionC		memRegions[CHIPSET_MEMORY_NREGIONS];
	pageTableCacheC		pageTableCache;

	/* 'defaultConfig' is the default used for any call to
	 * fragmentedGetFrames(). It is also copied to any new process being
	 * spawned. Note that I used 'process' and not 'thread'. When a process
	 * has a thread 0, then any other threads it spawns will derive their
	 * configuration from thread 0 of that process.
	 *
	 * 'sharedBank' is the bank ID of a bank which is used to hold all CPUs
	 * which were not explicitly listed as belonging to a certain bank, and
	 * all memory ranges which were not listed to be on any bank.
	 *
	 * This is done in case a NUMA map has holes, such as a hole where a
	 * particular memory region is left out of the NUMA map, and there is
	 * no place that it has been listed into, or where a CPU is not listed
	 * as a member of a bank.
	 *
	 * On a NUMA build where no NUMA map is found, the kernel will create
	 * one NUMA Stream, and list that stream's ID as that of the
	 * "sharedBank".
	 **/
	// All numa banks with memory on them are listed here.
	hardwareIdListC		memoryBanks;
	localAffinityS		defaultAffinity;
	ubit32			nBanks;
};

extern memoryTribC		memoryTrib;


/**	Inline Methods
 ****************************************************************************/

inline error_t memoryTribC::pageTablePop(paddr_t *paddr)
{
	return pageTableCache.pop(paddr);
}

inline void memoryTribC::pageTablePush(paddr_t paddr)
{
	pageTableCache.push(paddr);
}

#if __SCALING__ < SCALING_CC_NUMA
inline numaMemoryBankC *memoryTribC::getBank(numaBankId_t bankId)
{
	return static_cast<numaMemoryBankC *>(
		memoryBanks.getItem(defaultAffinity.def.rsrc) );
}
#else
inline numaMemoryBankC *memoryTribC::getBank(numaBankId_t bankId)
{
	return static_cast<numaMemoryBankC *>( memoryBanks.getItem(bankId) );
}
#endif

#endif

