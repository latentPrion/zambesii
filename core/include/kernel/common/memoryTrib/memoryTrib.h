#ifndef _MEMORY_TRIB_H
	#define _MEMORY_TRIB_H

	#include <scaling.h>
	#include <arch/paddr_t.h>
	#include <arch/paging.h>
	#include <chipset/__kmemory.h>
	#include <chipset/regionMap.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kcxxlib/cstring>
	#include <__kclasses/memBmp.h>
	#include <__kclasses/pageTableCache.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/memoryRegion.h>
	#include <kernel/common/numaTrib/numaStream.h>
	#include <kernel/common/memoryTrib/memoryStream.h>

class memoryTribC
:
public tributaryC
{
public:
	memoryTribC(
		paddr_t __kspaceBase, paddr_t __kspaceSize,
		void *__kspaceInitMem,
		pagingLevel0S *level0Accessor, paddr_t level0Paddr);

	error_t initialize(
		void *swampStart, uarch_t swampSize,
		vSwampC::holeMapS *holeMap);
	
public:
	void *__kspaceMemAlloc(uarch_t nPages);
	void __kspaceMemFree(void *vaddr, uarch_t nPages);

	void *rawMemAlloc(uarch_t nPages);
	void rawMemFree(void *vaddr, uarch_t nPages);

	error_t pageTablePop(paddr_t *paddr);
	void pageTablePush(paddr_t paddr);

public:
	memoryStreamC		__kmemoryStream;
	memBmpC			__kspaceBmp;

private:
	memoryRegionC		memRegions[CHIPSET_MEMORY_NREGIONS];
	pageTableCacheC		pageTableCache;
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

#endif

