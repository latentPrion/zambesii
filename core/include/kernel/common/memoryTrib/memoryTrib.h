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

#define MEMTRIB		"Memory Trib: "

#define MEMTRIB_GETFRAMES_FLAGS_NOSLEEP		(1<<0)

class memoryTribC
:
public tributaryC
{
public:
	memoryTribC(void);

	error_t initialize(void);
	error_t __kspaceInitialize(void);
	error_t pmemInit(void);
	error_t memRegionInit(void);

public:
	void *rawMemAlloc(uarch_t nPages, uarch_t flags);
	void rawMemFree(void *vaddr, uarch_t nPages);

	error_t pageTablePop(paddr_t *paddr);
	void pageTablePush(paddr_t paddr);

	numaMemoryBankC *getBank(numaBankId_t bankId);
	error_t createBank(
		numaBankId_t bankId, numaMemoryBankC *preAllocated=__KNULL);

	void destroyBank(numaBankId_t bankId);

	/* NOTE:
	 * These next few functions are an exposure of the underlying
	 * numaMemoryBankC interfaces on each bank. The kernel can efficiently
	 * tell how best to serve an allocation most of the time. Therefore
	 * developers are encouraged to use these top-level functions and let
	 * the implementation logic behind the Memory Trib decide what's best.
	 *
	 * Otherwise you end up with NUMA bank specifics in the kernel.
	 **/
	// Physical memory management functions.
	status_t fragmentedGetFrames(
		uarch_t nFrames, paddr_t *ret, ubit32 flags=0);

	error_t contiguousGetFrames(
		uarch_t nFrames, paddr_t *ret, ubit32 flags=0);

	void releaseFrames(paddr_t paddr, uarch_t nFrames);

	void mapRangeUsed(paddr_t baseAddr, uarch_t nFrames);
	void mapRangeUnused(paddr_t baseAddr, uarch_t nFrames);

	void dump(void);

private:

	void init2_spawnNumaStreams(zkcmNumaMapS *map);
	void init2_generateNumaMemoryRanges(
		zkcmNumaMapS *map, sarch_t *__kspaceBool);

	void init2_generateShbankFromNumaMap(
		zkcmMemConfigS *cfg, zkcmNumaMapS *map, sarch_t *__kspaceBool);

public:
	bitmapC			availableBanks;

private:
	memoryRegionC		memRegions[CHIPSET_MEMORY_NREGIONS];
	pageTableCacheC		pageTableCache;

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
	hardwareIdListC		memoryBanks;
#if __SCALING__ < SCALING_CC_NUMA
	sharedResourceGroupC<multipleReaderLockC, numaBankId_t>
		defaultMemoryBank;
#endif
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
		memoryBanks.getItem(defaultMemoryBank.rsrc) );
}
#else
inline numaMemoryBankC *memoryTribC::getBank(numaBankId_t bankId)
{
	if (bankId != NUMABANKID_INVALID)
	{
		return static_cast<numaMemoryBankC *>(
			memoryBanks.getItem(bankId) );
	}
	else {
		return __KNULL;
	};
}
#endif

#endif

