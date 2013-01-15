#ifndef _MEMORY_STREAM_H
	#define _MEMORY_STREAM_H

	#include <arch/paging.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/allocTable.h>
	#include <kernel/common/stream.h>
	#include <kernel/common/vSwamp.h>
	#include <kernel/common/memoryTrib/vaddrSpaceStream.h>
	#include <kernel/common/memoryTrib/allocFlags.h>

/**	EXPLANATION:
 * The Memory Stream is responsible for the per-process monitoring of all
 * memory allocations etc.
 *
 * The Memory Stream for a process provides the allocation table needed to
 * ensure that the kernel's low level garbage collection will work when the
 * process exits. It also provides a cache of allocations so that we don't have
 * constant contention over the NUMA Tributary for physical memory.
 *
 * Each process must have its Virtual Address Space managed; The Memory Stream
 * contains the vaddrSpaceStreamC within itself to provide this functionality.
 *
 * NOTE: When this class reaches the pique of its refinement, the idea is that
 * since each process will have its own private memory manager, in the form of
 * this class, the Memory Stream, the kernel can do memory profiling on a
 * process, and thereby decide how best to schedule/allocate, etc for that
 * process so that it is best catered for.
 **/

/**	EXPLANATION:
 * MEMORYSTREAM_COMMIT_MAX_NFRAMES is the memory allocation request size above
 * which we begin fake mapping.
 *
 * Zambesii will never give out more than MEMORYSTREAM_COMMIT_MAX_NFRAMES + some
 * extra pages in one allocation unless MEMALLOC_NO_FAKEMAP is passed along
 * with the allocation request.
 *
 * To be precise, the Memory Stream will, on seeing a request for more pages
 * than the max frame commit size, calculate the number of frames over the max
 * commit size to give out as a quarter of the number of pages over the max
 * commit size.
 *
 * All other pages in the range beyond the "grace" extension (the quarter) are
 * fakemapped to generate a translation fault when the app touches them.
 *
 * The reason for this is to ensure that huge amounts of physical memory aren't
 * dispatched to a process all at once. When a process allocates a large amount
 * of memory, it's unlikely it'll be using all at once, so we wait until the
 * process touches the memory to commit physical memory to it.
 **/
#define MEMORYSTREAM_COMMIT_MAX_NFRAMES				\
	(PAGING_BYTES_TO_PAGES(0x180000))

#define MEMORYSTREAM_FAKEMAP_PAGE_TRANSFORM(np)		\
	((np > MEMORYSTREAM_COMMIT_MAX_NFRAMES) \
		? ((np - MEMORYSTREAM_COMMIT_MAX_NFRAMES) / 4) \
			+ MEMORYSTREAM_COMMIT_MAX_NFRAMES \
		: np)

#define MEMORYSTREAM		"Memory Stream "

class memoryTribC;

class memoryStreamC
:
public streamC
{
friend class memoryTribC;

public:
	// Used to initialize userspace process streams.
	memoryStreamC(
		uarch_t id,
		void *swampStart, uarch_t swampSize,
		vSwampC::holeMapS *holeMap,
		pagingLevel0S *level0Accessor, paddr_t level0Paddr);

	// Used to initialize the kernel stream.
	memoryStreamC(uarch_t id,
		pagingLevel0S *level0Accessor, paddr_t level0Paddr);

	error_t initialize(
		void *swampStart, uarch_t swampSize,
		vSwampC::holeMapS *holeMap);

public:
	void *memAlloc(uarch_t nPages, uarch_t flags=0);
	void memFree(void *vaddr);

	// These two make use of the allocTableC to store nPages.
	void *memRegionAlloc(ubit8 regionId, uarch_t nPages);
	void memRegionFree(ubit8 regionId, void *vaddr);

public:
	void cut(void);
	error_t bind(void);
	void dump(void);

private:
	stackCacheC<void *>	allocCache;
public:
	vaddrSpaceStreamC	vaddrSpaceStream;
	allocTableC		allocTable;
};

#endif

