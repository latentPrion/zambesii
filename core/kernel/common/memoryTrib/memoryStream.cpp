
#include <scaling.h>
#include <arch/paddr_t.h>
#include <arch/walkerPageRanger.h>
#include <lang/lang.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/pageAttributes.h>
#include <kernel/common/task.h>
#include <kernel/common/processId.h>
#include <kernel/common/numaConfig.h>
#include <kernel/common/process.h>
#include <kernel/common/panic.h>
#include <kernel/common/memoryTrib/memoryStream.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/numaTrib/numaTrib.h>

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
	binding.lock.acquire();

	memAlloc = &memoryStreamC::real_memAlloc;
	binding.rsrc = 1;
	vaddrSpaceStream.bind();

	binding.lock.release();
}

void memoryStreamC::cut(void)
{
	binding.lock.acquire();

	memAlloc = &memoryStreamC::dummy_memAlloc;
	binding.rsrc = 0;
	vaddrSpaceStream.cut();

	binding.lock.release();
}

void memoryStreamC::dump(void)
{
	__kdebug.printf(NOTICE"Memory Stream %X: Dumping.\n", id);
	vaddrSpaceStream.dump();
}

void *memoryStreamC::dummy_memAlloc(uarch_t, uarch_t)
{
	return __KNULL;
}

void *memoryStreamC::real_memAlloc(uarch_t nPages, uarch_t flags)
{
	void		*ret;
	uarch_t		pos, pmapFlags;
	status_t	nFrames, totalFrames=0, nMapped;
	paddr_t		paddr, status;
	error_t		err;

	if (nPages == 0) {
		return __KNULL;
	};

	// Try to allocate from the cache.
	if (allocCache.pop(nPages, &ret) == ERROR_SUCCESS)
	{
		__kdebug.printf(NOTICE"Memory Stream %X: getPages(%d): "
			"allocating from cache: v %p.\n", id, nPages, ret);

		walkerPageRanger::setAttributes(
			&vaddrSpaceStream.vaddrSpace,
			ret, nPages, WPRANGER_OP_SET_PRESENT, 0);

		return ret;
	};

	ret = (vaddrSpaceStream.*vaddrSpaceStream.getPages)(nPages);
	if (ret == __KNULL) {
		return __KNULL;
	};

	pos = reinterpret_cast<uarch_t>( ret );

	if (__KFLAG_TEST(flags, MEMALLOC_NO_FAKEMAP))
	{
		/* You can put a counter here to track how many times we've been
		 * round this loop. Maybe cut off after a certain number of
		 * tries with nFrames returned as 0, and just unmap the vmem and
		 * return NULL.
		 **/
		for (; totalFrames < static_cast<status_t>( nPages ); )
		{
			nFrames = numaTrib.fragmentedGetFrames(nPages, &paddr);
			if (nFrames > 0)
			{
				nMapped = walkerPageRanger::mapInc(
					&vaddrSpaceStream.vaddrSpace,
					reinterpret_cast<void *>( pos ),
					paddr, nFrames,
					PAGEATTRIB_PRESENT
					| PAGEATTRIB_WRITE
					| ((id == __KPROCESSID)
						? PAGEATTRIB_SUPERVISOR
						: 0));

				if (nMapped < nFrames) {
					/* FIXME: Pmem leak here. */
					goto unmapVRangePartial;
				};

				totalFrames += nFrames;
				pos += nFrames * PAGING_BASE_SIZE;
			};
		};
	}
	else
	{
		nFrames = numaTrib.fragmentedGetFrames(nPages, &paddr);
		if (nFrames < 1) {
			goto releaseVmem;
		};

		nMapped = walkerPageRanger::mapInc(
			&vaddrSpaceStream.vaddrSpace,
			reinterpret_cast<void *>( pos ),
			paddr, nFrames,
			PAGEATTRIB_PRESENT | PAGEATTRIB_WRITE
			| ((id == __KPROCESSID) ? PAGEATTRIB_SUPERVISOR
				: 0));

		if (nMapped < nFrames)
		{
			numaTrib.releaseFrames(paddr, nFrames);
			goto unmapVRangePartial;
		};

		pos += totalFrames * PAGING_BASE_SIZE;

		// Fakemap from pos onwards.
		nMapped = walkerPageRanger::mapNoInc(
			&vaddrSpaceStream.vaddrSpace,
			reinterpret_cast<void *>( pos ),
			PAGING_LEAF_FAKEMAPPED,
			nPages - nFrames,
			PAGEATTRIB_WRITE
			| ((id == __KPROCESSID) ? PAGEATTRIB_SUPERVISOR : 0));

		if (nMapped < (static_cast<status_t>( nPages ) - nFrames))
		{
			numaTrib.releaseFrames(paddr, nFrames);
			totalFrames = nFrames;
			goto unmapVRangePartial;
		};
	};

	// At this point, ret is backed by pmem and fake mapped possibly.
	err = allocTable.addEntry(ret, nPages, ALLOCTABLE_TYPE_DATA, 0);
	if (err != ERROR_SUCCESS) {
		goto unmapVRangeFull;
	};

	// Allocation successful.
	return ret;

unmapVRangePartial:
	for (uarch_t nPageCount = totalFrames + nMapped;
		nPageCount > 0;
		nPageCount--)
	{
		status = walkerPageRanger::unmap(
			&vaddrSpaceStream.vaddrSpace,
			ret, &paddr, 1, &pmapFlags);

		if (status == WPRANGER_STATUS_BACKED) {
			numaTrib.releaseFrames(paddr, 1);
		};
	};
	vaddrSpaceStream.releasePages(ret, nPages);
	return __KNULL;

unmapVRangeFull:
	for (uarch_t nPageCount = nPages;
		nPageCount > 0;
		nPageCount--)
	{
		status = walkerPageRanger::unmap(
			&vaddrSpaceStream.vaddrSpace,
			ret, &paddr, 1, &pmapFlags);

		if (status == WPRANGER_STATUS_BACKED) {
			numaTrib.releaseFrames(paddr, 1);
		};
	};

releaseVmem:
	vaddrSpaceStream.releasePages(ret, nPages);
	return __KNULL;
}

void memoryStreamC::memFree(void *vaddr)
{
	paddr_t		paddr;
	error_t		err;
	status_t	status;
	uarch_t		nPages, _nPages, tracker, unmapFlags;
	ubit8		type, flags;

	if (vaddr == __KNULL) { return; };

	// First ask the alloc table if the alloc is valid.
	err = allocTable.lookup(vaddr, &nPages, &type, &flags);
	if (err != ERROR_SUCCESS) {
		return;
	};

	// Attempt to just push the allocation whole into the cache.
	if (allocCache.push(nPages, vaddr) == ERROR_SUCCESS)
	{
		__kdebug.printf(NOTICE"Memory Stream %X: memFree(%p): "
			"(nPages = %d): free to cache.\n", id, vaddr, nPages);

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
			numaTrib.releaseFrames(paddr, nPages);
		};
	};

	// Free the virtual memory.
	vaddrSpaceStream.releasePages(vaddr, nPages);
	// Remove the entry from the process's Alloc Table.
	allocTable.removeEntry(vaddr);	
}

