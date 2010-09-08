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
 * bitmaps themselves are functional from very early on, each memoryRegionC
 * has its own allocTableC.
 *
 * To allocate from a memory region, you make a call to
 * memoryRegionC::*GetFrames(). This call is going to be making use of the alloc
 * table in the memory region class object. So what happens when allocTableC
 * calls memoryTribC::rawMemAlloc()?
 *
 * Nothing good. If a call for a memory region allocation is made before the
 * NUMA Tributary has been successfully initialized, then the behaviour of the
 * memory region allocator is undefined, and likely to be destructive.
 **/

typedef ubit8		memoryRegionId_t;

class memoryTribC;

class memoryRegionC
{
friend class memoryTribC;

public:
	memoryRegionC(void)
	:
	info(__KNULL), memBmp(__KNULL)
	{};

public:
	error_t contiguousGetFrames(uarch_t nFrames, paddr_t *paddr);
	error_t fragmentedGetFrames(uarch_t nFrames, paddr_t *paddr);
	void releaseFrames(paddr_t paddr, uarch_t nFrames);

private:
	chipsetRegionMapEntryS	*info;
	memBmpC			*memBmp;
	allocTableC		allocTable;
};

#endif

