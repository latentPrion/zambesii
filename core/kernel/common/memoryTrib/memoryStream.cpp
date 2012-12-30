
#include <debug.h>

#include <scaling.h>
#include <arch/paddr_t.h>
#include <arch/walkerPageRanger.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/pageAttributes.h>
#include <kernel/common/processId.h>
#include <kernel/common/panic.h>
#include <kernel/common/memoryTrib/memoryStream.h>
#include <kernel/common/memoryTrib/pmmBridge.h>
#include <kernel/common/memoryTrib/allocFlags.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


// Constructor for userspace streams.
memoryStreamC::memoryStreamC(
	uarch_t id,
	void *swampStart, uarch_t swampSize,
	vSwampC::holeMapS *holeMap,
	pagingLevel0S *level0Accessor, paddr_t level0Paddr
	)
:
streamC(id),
vaddrSpaceStream(
	id, swampStart, swampSize, holeMap, level0Accessor, level0Paddr)
{
	bind();
}

// Constructor for the kernel stream.
memoryStreamC::memoryStreamC(uarch_t id,
	pagingLevel0S *level0Accessor, paddr_t level0Paddr
	)
:
streamC(id),
vaddrSpaceStream(id, level0Accessor, level0Paddr)
{
	bind();
}

error_t memoryStreamC::initialize(
	void *swampStart, uarch_t swampSize, vSwampC::holeMapS *holeMap
	)
{
	return vaddrSpaceStream.initialize(swampStart, swampSize, holeMap);
}

void memoryStreamC::bind(void)
{
}

void memoryStreamC::cut(void)
{
}

void memoryStreamC::dump(void)
{
	__kprintf(NOTICE MEMORYSTREAM"%d: Dumping.\n", id);
	vaddrSpaceStream.dump();
}

void *memoryStreamC::memAlloc(uarch_t nPages, uarch_t flags)
{
	uarch_t		commit=nPages, ret, f, pos, nTries, localFlush=0;
	status_t	totalFrames, nFrames, nMapped;
	paddr_t		p;

	if (nPages == 0) {
		return __KNULL;
	};

	if (__KFLAG_TEST(flags, MEMALLOC_LOCAL_FLUSH_ONLY)) { localFlush = 1; };

	// Try alloc cache.
	if ((!__KFLAG_TEST(flags, MEMALLOC_PURE_VIRTUAL))
		&& (allocCache.pop(nPages, reinterpret_cast<void **>( &ret ))
		== ERROR_SUCCESS))
	{
		walkerPageRanger::setAttributes(
			&vaddrSpaceStream.vaddrSpace,
			(void *)ret, nPages, WPRANGER_OP_SET_PRESENT,
			(localFlush) ? PAGEATTRIB_LOCAL_FLUSH_ONLY : 0);

		return reinterpret_cast<void *>( ret );
	};

	// Calculate the number of frames to commit before fakemapping.
	if (!__KFLAG_TEST(flags, MEMALLOC_NO_FAKEMAP)) {
		commit = MEMORYSTREAM_FAKEMAP_PAGE_TRANSFORM(nPages);
	};
	if (__KFLAG_TEST(flags, MEMALLOC_PURE_VIRTUAL)) {
		commit = 0;
	};

	ret = reinterpret_cast<uarch_t>( vaddrSpaceStream.getPages(nPages) );

	if (ret == __KNULL) {
		return __KNULL;
	};

	for (totalFrames=0, nTries=0;
		totalFrames < static_cast<sarch_t>( commit ); )
	{
#if __SCALING__ >= SCALING_CC_NUMA
		nFrames = memoryTribPmm::configuredGetFrames(
			&cpuTrib.getCurrentCpuStream()
				->taskStream.currentTask->localAffinity,
			commit - totalFrames, &p);
if (pp==1){ __kprintf(FATAL"memAlloc: Tracing. Vaddr from alloc 0x%p, nPages %d, pmem nFrames: %d.\n", ret, nPages, nFrames); };
#else
		nFrames = memoryTribPmm::fragmentedGetFrames(
			commit - totalFrames, &p);
#endif
		if (nFrames > 0)
		{
			nMapped = walkerPageRanger::mapInc(
				&vaddrSpaceStream.vaddrSpace,
				reinterpret_cast<void *>(
					ret + (totalFrames
						<< PAGING_BASE_SHIFT) ),
				p,
				nFrames,
				PAGEATTRIB_WRITE | PAGEATTRIB_PRESENT
				| ((this->id == __KPROCESSID)
					? PAGEATTRIB_SUPERVISOR : 0)
				| ((localFlush)?PAGEATTRIB_LOCAL_FLUSH_ONLY:0));

			totalFrames += nFrames;

			if (nMapped < nFrames)
			{
				__kprintf(WARNING MEMORYSTREAM"0x%X: "
					"WPR failed to map %d pages for alloc "
					"of %d frames.\n",
					this->id, nFrames, nPages);

				goto releaseAndUnmap;
			};
		}
		else
		{
			// Make sure we don't waste time spinning indefinitely.
			nTries++;
			if (nTries >= 4)
			{
				__kprintf(NOTICE MEMORYSTREAM"%d: memAlloc(%d):"
					" with commit nFrames %d: No pmem.\n",
					this->id, nPages, commit);

				goto releaseAndUnmap;
			};
		};
	};

	// Now see how many frames we must fake map. 'totalFrames' holds this.
	if (!__KFLAG_TEST(flags, MEMALLOC_NO_FAKEMAP))
	{
		nMapped = walkerPageRanger::mapNoInc(
			&vaddrSpaceStream.vaddrSpace,
			reinterpret_cast<void *>(
				ret + (totalFrames << PAGING_BASE_SHIFT) ),
			PAGESTATUS_FAKEMAPPED_DYNAMIC
				<< PAGING_PAGESTATUS_SHIFT,
			nPages - totalFrames,
			PAGEATTRIB_WRITE
			| ((this->id == __KPROCESSID) ? PAGEATTRIB_SUPERVISOR
				: 0)
			| ((localFlush) ? PAGEATTRIB_LOCAL_FLUSH_ONLY : 0));

		if (nMapped < static_cast<sarch_t>( nPages ) - totalFrames)
		{
			__kprintf(WARNING MEMORYSTREAM"0x%X: WPR failed to "
				"fakemap %d pages for alloc of %d pages.\n",
				this->id, nPages - totalFrames, nPages);

			goto releaseAndUnmap;
		};
	};
	// Now add to alloc table.
	if (allocTable.addEntry(reinterpret_cast<void *>( ret ), nPages, 0)
		== ERROR_SUCCESS)
	{
		return reinterpret_cast<void *>( ret );
	};

	// If the alloc table add failed, then unwind and undo everything.

releaseAndUnmap:
	// Release all of the pmem so far.
	pos = ret;
	while (totalFrames > 0)
	{
		__KFLAG_SET(f, PAGEATTRIB_LOCAL_FLUSH_ONLY);
		nMapped = walkerPageRanger::unmap(
			&vaddrSpaceStream.vaddrSpace,
			reinterpret_cast<void *>( pos ),
			&p, 1, &f);

		if (nMapped == WPRANGER_STATUS_BACKED) {
			memoryTribPmm::releaseFrames(p, 1);
		};

		pos += PAGING_BASE_SIZE;
		totalFrames--;
	};

	vaddrSpaceStream.releasePages(reinterpret_cast<void *>( ret ), nPages);

	return __KNULL;
}

void memoryStreamC::memFree(void *vaddr)
{
	paddr_t		paddr;
	error_t		err;
	status_t	status;
	uarch_t		nPages, _nPages, tracker, unmapFlags;
	ubit8		flags;

	if (vaddr == __KNULL) { return; };

	/* FIXME: Be careful when freeing a process: it is possible to double
	 * free a memory area if it is pushed into the alloc cache.
	 **/

	// First ask the alloc table if the alloc is valid.
	err = allocTable.lookup(vaddr, &nPages, &flags);
	if (err != ERROR_SUCCESS) {
		return;
	};

	// Attempt to just push the allocation whole into the cache.
	if (allocCache.push(nPages, vaddr) == ERROR_SUCCESS)
	{
		walkerPageRanger::setAttributes(
			&vaddrSpaceStream.vaddrSpace,
			vaddr, nPages, WPRANGER_OP_CLEAR_PRESENT, 0);

		return;
	};

	tracker = reinterpret_cast<uarch_t>( vaddr );
	_nPages = nPages;
	for (; _nPages > 0; _nPages--, tracker+=PAGING_BASE_SIZE)
	{
		status = walkerPageRanger::unmap(
			&vaddrSpaceStream.vaddrSpace,
			reinterpret_cast<void *>( tracker ),
			&paddr, 1, &unmapFlags);

		if (status == WPRANGER_STATUS_BACKED) {
			memoryTribPmm::releaseFrames(paddr, nPages);
		};
	};

	// Free the virtual memory.
	vaddrSpaceStream.releasePages(vaddr, nPages);
	// Remove the entry from the process's Alloc Table.
	allocTable.removeEntry(vaddr);	
}

