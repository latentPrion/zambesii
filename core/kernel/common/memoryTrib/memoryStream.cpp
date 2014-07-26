
#include <scaling.h>
#include <arch/paddr_t.h>
#include <arch/walkerPageRanger.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/pageAttributes.h>
#include <kernel/common/processId.h>
#include <kernel/common/process.h>
#include <kernel/common/panic.h>
#include <kernel/common/memoryTrib/memoryStream.h>
#include <kernel/common/memoryTrib/pmmBridge.h>
#include <kernel/common/memoryTrib/allocFlags.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


error_t MemoryStream::bind(void)
{
	return ERROR_SUCCESS;
}

void MemoryStream::cut(void)
{
}

void MemoryStream::dump(void)
{
	printf(NOTICE MEMORYSTREAM"%d: Dumping.\n", id);
}

void *MemoryStream::memAlloc(uarch_t nPages, uarch_t flags)
{
	uarch_t		commit=nPages, f, nTries, localFlush=0;
	status_t	totalFrames, nFrames, nMapped;
	paddr_t		p;
	ubit8		fKernel;
	void		*ret;

	if (nPages == 0) { return NULL; };

	if (FLAG_TEST(flags, MEMALLOC_LOCAL_FLUSH_ONLY)) { localFlush = 1; };

	// Try alloc cache.
	if (!FLAG_TEST(flags, MEMALLOC_PURE_VIRTUAL)
		&& (allocCache.pop(nPages, (void **)&ret) == ERROR_SUCCESS))
	{
		walkerPageRanger::setAttributes(
			&parent->getVaddrSpaceStream()->vaddrSpace,
			(void *)ret, nPages, WPRANGER_OP_SET_PRESENT,
			(localFlush) ? PAGEATTRIB_LOCAL_FLUSH_ONLY : 0);

		return ret;
	};

	// Calculate the number of frames to commit before fakemapping.
	if (FLAG_TEST(flags, MEMALLOC_PURE_VIRTUAL)) { commit = 0; };
	if (!FLAG_TEST(flags, MEMALLOC_NO_FAKEMAP)
		&& !FLAG_TEST(flags, MEMALLOC_PURE_VIRTUAL))
	{
		commit = MEMORYSTREAM_FAKEMAP_PAGE_TRANSFORM(nPages);
	};

	ret = parent->getVaddrSpaceStream()->getPages(nPages);
	if (ret == NULL) { return NULL; };

	fKernel = (parent->execDomain == PROCESS_EXECDOMAIN_KERNEL) ? 1 : 0;

	for (totalFrames=0, nTries=0; totalFrames < (sarch_t)commit; )
	{
		nFrames = memoryTribPmm::fragmentedGetFrames(
			commit - totalFrames, &p);

		if (nFrames > 0)
		{
			nMapped = walkerPageRanger::mapInc(
				&parent->getVaddrSpaceStream()->vaddrSpace,
				(void *)((uintptr_t)ret
					+ (totalFrames << PAGING_BASE_SHIFT)),
				p, nFrames,
				PAGEATTRIB_PRESENT
				| PAGEATTRIB_WRITE
				| ((fKernel) ? PAGEATTRIB_SUPERVISOR : 0)
				| ((localFlush)?PAGEATTRIB_LOCAL_FLUSH_ONLY:0));

			totalFrames += nFrames;

			if (nMapped < nFrames)
			{
				printf(WARNING MEMORYSTREAM"0x%x: "
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
				printf(NOTICE MEMORYSTREAM"%d: memAlloc(%d):"
					" with commit nFrames %d: No pmem.\n",
					this->id, nPages, commit);

				goto releaseAndUnmap;
			};
		};
	};

	// Now see how many frames we must fake map. 'totalFrames' holds this.
	if (!FLAG_TEST(flags, MEMALLOC_NO_FAKEMAP))
	{
		nMapped = walkerPageRanger::mapNoInc(
			&parent->getVaddrSpaceStream()->vaddrSpace,
			(void *)((uintptr_t)ret
				+ (totalFrames << PAGING_BASE_SHIFT)),
			PAGESTATUS_FAKEMAPPED_DYNAMIC <<PAGING_PAGESTATUS_SHIFT,
			nPages - totalFrames,
			PAGEATTRIB_WRITE
			| ((fKernel) ? PAGEATTRIB_SUPERVISOR : 0)
			| ((localFlush) ? PAGEATTRIB_LOCAL_FLUSH_ONLY : 0));

		if (nMapped < static_cast<sarch_t>( nPages ) - totalFrames)
		{
			printf(WARNING MEMORYSTREAM"0x%X: WPR failed to "
				"fakemap %d pages for alloc of %d pages.\n",
				this->id, nPages - totalFrames, nPages);

			goto releaseAndUnmap;
		};
	};

	// Now add to alloc table.
	if (allocTable.addEntry((void *)ret, nPages, 0)	== ERROR_SUCCESS) {
		return ret;
	};

	// If the alloc table add failed, then unwind and undo everything.

releaseAndUnmap:
	void		*pos;

	// Release all of the pmem so far.
	pos = ret;
	while (totalFrames > 0)
	{
		/**	FIXME: Analyse this.
		**/
		f = 0;
		if (localFlush){ FLAG_SET(f, PAGEATTRIB_LOCAL_FLUSH_ONLY); };
		nMapped = walkerPageRanger::unmap(
			&parent->getVaddrSpaceStream()->vaddrSpace,
			pos,
			&p, 1, &f);

		if (nMapped == WPRANGER_STATUS_BACKED) {
			memoryTribPmm::releaseFrames(p, 1);
		};

		pos = (void *)((uintptr_t)pos + PAGING_BASE_SIZE);
		totalFrames--;
	};

	parent->getVaddrSpaceStream()->releasePages(ret, nPages);
	return NULL;
}

void MemoryStream::memFree(void *vaddr)
{
	paddr_t		paddr;
	error_t		err;
	status_t	status;
	uarch_t		nPages, _nPages, tracker, unmapFlags;
	ubit8		flags;

	if (vaddr == NULL) { return; };

	/* FIXME: Be careful when freeing a process: it is possible to double
	 * free a memory area if it is pushed into the alloc cache.
	 **/

	// First ask the alloc table if the alloc is valid.
	err = allocTable.lookup(vaddr, &nPages, &flags);
	if (err != ERROR_SUCCESS) {
		return;
	};

	if (allocCache.push(nPages, vaddr) == ERROR_SUCCESS)
	{
		walkerPageRanger::setAttributes(
			&parent->getVaddrSpaceStream()->vaddrSpace,
			vaddr, nPages, WPRANGER_OP_CLEAR_PRESENT, 0);

		return;
	};

	tracker = reinterpret_cast<uarch_t>( vaddr );
	_nPages = nPages;
	for (; _nPages > 0; _nPages--, tracker+=PAGING_BASE_SIZE)
	{
		status = walkerPageRanger::unmap(
			&parent->getVaddrSpaceStream()->vaddrSpace,
			reinterpret_cast<void *>( tracker ),
			&paddr, 1, &unmapFlags);

		if (status == WPRANGER_STATUS_BACKED) {
			memoryTribPmm::releaseFrames(paddr, nPages);
		};
	};

	// Free the virtual memory.
	parent->getVaddrSpaceStream()->releasePages(vaddr, nPages);
	// Remove the entry from the process's Alloc Table.
	allocTable.removeEntry(vaddr);	
}

