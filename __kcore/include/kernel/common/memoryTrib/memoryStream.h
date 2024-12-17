#ifndef _MEMORY_STREAM_H
	#define _MEMORY_STREAM_H

	#include <arch/paging.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/allocTable.h>
	#include <chipset/zkcm/zkcmCore.h>
	#include <kernel/common/stream.h>
	#include <kernel/common/vSwamp.h>
	#include <kernel/common/memoryTrib/vaddrSpaceStream.h>
	#include <kernel/common/memoryTrib/allocFlags.h>

/**	EXPLANATION:
 * The Memory Stream is responsible for the per-process monitoring of all
 * dynamic memory allocations. A process' Memory Stream provides the allocation
 * table needed to ensure the kernel's low level garbage collection will work
 * when a process exits and also provides a cache of allocations to reduce
 * contention over Memory Tributary for pmem.
 *
 * NOTE: When this class reaches the pique of its refinement, the idea is that
 * since each process will have its own private memory manager in the form of
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

class MemoryTrib;
class ProcessStream;
class Heap;

class MemoryStream
:
public Stream<ProcessStream>
{
friend class MemoryTrib;
friend class Heap;

public:
	MemoryStream(processId_t id, ProcessStream *parent)
	: Stream<ProcessStream>(parent, id)
	{}

	error_t initialize(void)
	{
		error_t		ret;

		ret = allocCache.initialize();
		if (ret != ERROR_SUCCESS) { return ret; };
		ret = allocTable.initialize();
		if (ret != ERROR_SUCCESS) { return ret; };

		if (PROCID_PROCESS(id) == PROCID_PROCESS(__KPROCESSID))
		{
			zkcmCore.chipsetEventNotification(
				__KPOWER_EVENT___KMEMORY_STREAM_AVAIL, 0);
		}

		return ERROR_SUCCESS;
	}

public:
	// ONLY to be used for allocating dynamic memory and stacks.
	void *memAlloc(uarch_t nPages, uarch_t flags=0);
	void memFree(void *vaddr);
	void *memRealloc(
		void *oldmem, uarch_t oldNBytes, uarch_t newNBytes,
		uarch_t flags=0);

	// These two make use of the AllocTable to store nPages.
	void *memoryRegionAlloc(
		ubit8 regionId, uarch_t nPages, uarch_t flags=0);

	void memoryRegionFree(ubit8 regionId, void *vaddr);

	// See allocTable.h for the rest of this class's API.

	void cut(void);
	error_t bind(void);
	void dump(void);

private:
	StackCache<void *>	allocCache;
	AllocTable		allocTable;
};

#endif

