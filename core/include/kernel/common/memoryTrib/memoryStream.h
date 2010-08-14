#ifndef _MEMORY_STREAM_H
	#define _MEMORY_STREAM_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/allocTable.h>
	#include <kernel/common/stream.h>
	#include <kernel/common/vSwamp.h>
	#include <kernel/common/memoryTrib/vaddrSpaceStream.h>

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

#define MEMALLOC_NO_FAKEMAP		(1<<0)

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
	void *(memoryStreamC::*memAlloc)(uarch_t nPages, uarch_t flags);
	void memFree(void *vaddr);

	// These two make use of the allocTableC to store nPages.
	void *(memoryStreamC::*memRegionAlloc)(ubit8 regionId, uarch_t nPages);
	void memRegionFree(ubit8 regionId, void *vaddr);

public:
	void cut(void);
	void bind(void);
	void dump(void);

	void *real_memAlloc(uarch_t nPages, uarch_t flags=0);
	void *dummy_memAlloc(uarch_t nPages, uarch_t flags);
	void *real_memRegionAlloc(ubit8 regionId, uarch_t nPages);
	void *dummy_memRegionAlloc(ubit8 regionId, uarch_t nPages);

private:
	stackCacheC<void *>	allocCache;
public:
	vaddrSpaceStreamC	vaddrSpaceStream;
	allocTableC		allocTable;
};

#endif

