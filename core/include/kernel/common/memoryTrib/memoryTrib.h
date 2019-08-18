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
	#include <__kclasses/resizeableArray.h>
	#include <__kclasses/pageTableCache.h>
	#include <__kclasses/hardwareIdList.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/memoryRegion.h>
	#include <kernel/common/memoryTrib/memoryStream.h>

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

#define MEMTRIB		"Memory Trib: "

class MemoryTrib
:
public Tributary
{
public:
	MemoryTrib(void);

	error_t initialize(void);
	error_t __kspaceInitialize(void);
	error_t pmemInit(void);
	error_t memRegionInit(void);

public:
	void *rawMemAlloc(uarch_t nPages, uarch_t flags);
	void rawMemFree(void *vaddr, uarch_t nPages, uarch_t flags=0);

	error_t pageTablePop(paddr_t *paddr);
	void pageTablePush(paddr_t paddr);

	NumaMemoryBank *getBank(numaBankId_t bankId);
	error_t createBank(
		numaBankId_t bankId, NumaMemoryBank *preAllocated=NULL);

	void destroyBank(numaBankId_t bankId);

	/* NOTE:
	 * These next few functions are an exposure of the underlying
	 * NumaMemoryBank interfaces on each bank. The kernel can efficiently
	 * tell how best to serve an allocation most of the time. Therefore
	 * developers are encouraged to use these top-level functions and let
	 * the implementation logic behind the Memory Trib decide what's best.
	 *
	 * Otherwise you end up with NUMA bank specifics in the kernel.
	 **/
	// Physical memory management functions.
	enum fragmentedGetFramesE {
		FGF_FLAGS_NOSLEEP		= (1<<0)
	};
	status_t fragmentedGetFrames(
		uarch_t nFrames, paddr_t *ret, ubit32 flags=0);

	sbit8 releaseFrames(paddr_t paddr, uarch_t nFrames);
	void mapRangeUsed(paddr_t baseAddr, uarch_t nFrames);
	void mapRangeUnused(paddr_t baseAddr, uarch_t nFrames);

	void dump(void);

	/* These 2 functions should not be exposed to userspace: userspace
	 * should allocate and destroy scatter gather lists using
	 * FloodplainnStreams.
	 */
	enum constrainedGetFramesFlagsE {
		CGF_SGLIST_UNLOCKED		= (1<<0)
	};
	status_t constrainedGetFrames(
		fplainn::dma::constraints::Compiler *constraints,
		uarch_t nFrames,
		fplainn::dma::ScatterGatherList *retlist,
		ubit32 flags=0);

	sbit8 releaseFrames(fplainn::dma::ScatterGatherList *list);

private:

	void init2_spawnNumaStreams(sZkcmNumaMap *map);
	void init2_generateNumaMemoryRanges(
		sZkcmNumaMap *map, sarch_t *__kspaceBool);

	void init2_generateShbankFromNumaMap(
		sZkcmMemoryConfig *cfg, sZkcmNumaMap *map, sarch_t *__kspaceBool);

public:
	Bitmap			availableBanks;

private:
	MemoryRegion		memRegions[CHIPSET_MEMORY_NREGIONS];
	PageTableCache		pageTableCache;

	/* "defaultMemoryBank" is the bank that the kernel is currently using
	 * for general allocations.
	 *
	 * The "shared bank" is the bank ID of a bank which is used to hold all
	 * memory ranges which were not listed to be on any bank; e.g. where a
	 * NUMA map has holes, such as a hole where a particular memory region
	 * is usable, but left out of the NUMA map. Such a range of pmem will
	 * be added to the "shared bank".
	 *
	 * On a NUMA build where no NUMA map is found, the kernel will create
	 * one bank (the shared bank) and add all pmem to that bank.
	 **/
	ubit32			nBanks;
	HardwareIdList		memoryBanks;
#if __SCALING__ < SCALING_CC_NUMA
	SharedResourceGroup<MultipleReaderLock, numaBankId_t>
		defaultMemoryBank;
#endif
};

extern MemoryTrib		memoryTrib;


/**	Inline Methods
 ****************************************************************************/

inline error_t MemoryTrib::pageTablePop(paddr_t *paddr)
{
	return pageTableCache.pop(paddr);
}

inline void MemoryTrib::pageTablePush(paddr_t paddr)
{
	pageTableCache.push(paddr);
}

#if __SCALING__ < SCALING_CC_NUMA
inline NumaMemoryBank *MemoryTrib::getBank(numaBankId_t bankId)
{
	return static_cast<NumaMemoryBank *>(
		memoryBanks.getItem(defaultMemoryBank.rsrc) );
}
#else
inline NumaMemoryBank *MemoryTrib::getBank(numaBankId_t bankId)
{
	if (bankId != NUMABANKID_INVALID)
	{
		return static_cast<NumaMemoryBank *>(
			memoryBanks.getItem(bankId) );
	}
	else {
		return NULL;
	};
}
#endif

#endif

