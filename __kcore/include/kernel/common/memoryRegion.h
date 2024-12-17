#ifndef _MEMORY_REGION_H
	#define _MEMORY_REGION_H

	#include <arch/paddr_t.h>
	#include <chipset/regionMap.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/memBmp.h>
	#include <__kclasses/allocTable.h>

/**	EXPLANATION:
 * Take careful note of this. Now while the kernel spawns memory regions early
 * on, before proper memory detection has taken place, and the memory region
 * bitmaps themselves are functional from very early on, each MemoryRegion
 * has its own AllocTable.
 *
 * To allocate from a memory region, you make a call to
 * MemoryRegion::*GetFrames(). This call is going to be making use of the alloc
 * table in the memory region class object. So what happens when AllocTable
 * calls MemoryTrib::rawMemAlloc()?
 *
 * Nothing good. If a call for a memory region allocation is made before the
 * NUMA Tributary has been successfully initialized, then the behaviour of the
 * memory region allocator is undefined, and likely to be destructive.
 **/
#define MEMREGION	"MemoryRegion: "

typedef ubit8		memoryRegionId_t;

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

class MemoryTrib;

class MemoryRegion
{
friend class MemoryTrib;

public:
	MemoryRegion(void)
	:
	info(NULL), memBmp(NULL)
	{};

public:
	error_t fragmentedGetFrames(uarch_t nFrames, paddr_t *paddr);
	status_t constrainedGetFrames(
		fplainn::dma::constraints::Compiler *comCon,
		uarch_t nFrames,
		fplainn::dma::ScatterGatherList *retlist,
		uarch_t flags=0);
	/* Returns 1 if this MemoryRegion object was able to successfully take
	 * responsibility for freeing the allocation.
	 */
	sbit8 releaseFrames(paddr_t paddr, uarch_t nFrames);

private:
	sChipsetRegionMapEntry	*info;
	MemoryBmp		*memBmp;
};


/**	Inline methods:
 ******************************************************************************/

inline error_t MemoryRegion::fragmentedGetFrames(
	uarch_t nFrames, paddr_t *paddr
	)
{
	printf(FATAL MEMREGION"fragmentedGetFrames(%d, %P) unimplemented.\n",
		nFrames, paddr);

	return ERROR_UNIMPLEMENTED;
}

inline status_t MemoryRegion::constrainedGetFrames(
	fplainn::dma::constraints::Compiler *comCon,
	uarch_t nFrames,
	fplainn::dma::ScatterGatherList *retlist,
	uarch_t flags
	)
{
	/**	EXPLANATION:
	 * Just ask the bitmap to allocate.
	 */
	return memBmp->constrainedGetFrames(comCon, nFrames, retlist, flags);
}

inline sbit8 MemoryRegion::releaseFrames(paddr_t paddr, uarch_t nFrames)
{
	printf(FATAL MEMREGION"releaseFrames(%P, %d): unimplemented.\n",
		&paddr, nFrames);
	return 0;
}

#endif

