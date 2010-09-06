
#include <scaling.h>
#include <arch/paddr_t.h>
#include <arch/walkerPageRanger.h>
#include <lang/lang.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/pageAttributes.h>
#include <kernel/common/processId.h>
#include <kernel/common/panic.h>
#include <kernel/common/memoryTrib/memoryStream.h>
#include <kernel/common/memoryTrib/allocFlags.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/numaTrib/numaTrib.h>

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
	__kprintf(NOTICE"Memory Stream %X: Dumping.\n", id);
	vaddrSpaceStream.dump();
}

void *memoryStreamC::dummy_memAlloc(uarch_t, uarch_t)
{
	return __KNULL;
}

void *memoryStreamC::real_memAlloc(uarch_t nPages, uarch_t flags)
{
	uarch_t		commit=nPages, ret, f, pos;
	status_t	totalFrames, nFrames, nMapped;
	paddr_t		p;

	if (nPages == 0) {
		return __KNULL;
	};

	// Try alloc cache.
	if (!__KFLAG_TEST(flags, MEMALLOC_PURE_VIRTUAL)
		&& (allocCache.pop(nPages, reinterpret_cast<void **>( &ret ))
		== ERROR_SUCCESS))
	{
		return reinterpret_cast<void *>( ret );
	};

	// Calculate the number of frames to commit before fakemapping.
	if (!__KFLAG_TEST(flags, MEMALLOC_NO_FAKEMAP)) {
		commit = MEMORYSTREAM_FAKEMAP_PAGE_TRANSFORM(nPages);
	};

	ret = reinterpret_cast<uarch_t>(
		(vaddrSpaceStream.*vaddrSpaceStream.getPages)(nPages) );

	if (ret == __KNULL) {
		return __KNULL;
	};

	if (__KFLAG_TEST(flags, MEMALLOC_PURE_VIRTUAL)) {
		return reinterpret_cast<void *>( ret );
	};


	for (totalFrames=0; totalFrames < static_cast<sarch_t>( commit ); )
	{
#if __SCALING__ >= SCALING_CC_NUMA
		nFrames = numaTrib.configuredGetFrames(
			&cpuTrib.getCurrentCpuStream()->currentTask->numaConfig,
			commit - totalFrames, &p);
#else
		nFrames = numaTrib.fragmentedGetFrames(
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
					? PAGEATTRIB_SUPERVISOR : 0));

			totalFrames += nFrames;

			if (nMapped < nFrames)
			{
				__kprintf(WARNING MEMORYSTREAM"0x%X: "
					"WPR failed to map %d pages for alloc "
					"of %d frames.\n",
					this->id, nFrames, nPages);

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
				: 0));

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
		nMapped = walkerPageRanger::unmap(
			&vaddrSpaceStream.vaddrSpace,
			reinterpret_cast<void *>( pos ),
			&p, 1, &f);

		if (nMapped == WPRANGER_STATUS_BACKED) {
			numaTrib.releaseFrames(p, 1);
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
			numaTrib.releaseFrames(paddr, nPages);
		};
	};

	// Free the virtual memory.
	vaddrSpaceStream.releasePages(vaddr, nPages);
	// Remove the entry from the process's Alloc Table.
	allocTable.removeEntry(vaddr);	
}

