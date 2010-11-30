#ifndef _MEMORY_TRIB_H
	#define _MEMORY_TRIB_H

	#include <scaling.h>
	#include <arch/paddr_t.h>
	#include <arch/paging.h>
	#include <chipset/regionMap.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/memBmp.h>
	#include <__kclasses/pageTableCache.h>
	#include <__kclasses/hardwareIdList.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/memoryRegion.h>
	#include <kernel/common/memoryTrib/memoryStream.h>

class memoryTribC
:
public tributaryC
{
public:
	memoryTribC(pagingLevel0S *level0Accessor, paddr_t level0Paddr);

	error_t initialize(
		void *swampStart, uarch_t swampSize,
		vSwampC::holeMapS *holeMap);

	error_t initialize2(void);
	
public:
	void *rawMemAlloc(uarch_t nPages, uarch_t flags);
	void rawMemFree(void *vaddr, uarch_t nPages);

	error_t pageTablePop(paddr_t *paddr);
	void pageTablePush(paddr_t paddr);

	numaMemoryBankC *getBank(numaBankId_t bankId);
	error_t createBank(numaBankId_t bankId);
	void destroyBank(numaBankId_t bankId);

public:
	memoryStreamC		__kmemoryStream;

private:
	memoryRegionC		memRegions[CHIPSET_MEMORY_NREGIONS];
	pageTableCacheC		pageTableCache;

	// All numa banks with memory on them are listed here.
	hardwareIdListC		memoryBanks;
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

inline numaMemoryBankC *memoryTribC::getBank(numaBankId_t bankId)
{
	return static_cast<numaMemoryBankC *>( memoryBanks.getItem(bankId) );
}

#endif

