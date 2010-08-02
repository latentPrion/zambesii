
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

void *memoryStreamC::dummy_memAlloc(uarch_t)
{
	return __KNULL;
}

void *memoryStreamC::real_memAlloc(uarch_t nPages)
{
	paddr_t		paddr;
	void		*ret;
	status_t	nFound=0, nMapped;
	error_t	err;

	// Check the alloc cache for fast memory.
	if ((nPages == 1) && (allocCache.pop(nPages, &ret) == ERROR_SUCCESS))
	{
		walkerPageRanger::setAttributes(
			&vaddrSpaceStream.vaddrSpace,
			ret, nPages, WPRANGER_OP_SET_PRESENT, 0);

		__kdebug.printf(NOTICE"Memory Stream %X: memAlloc(%d): alloc "
			"from cache: vaddr %p\n",
			id, nPages, ret);

		return ret;
	}

	ret = (vaddrSpaceStream.*vaddrSpaceStream.getPages)(nPages);
	// See whether process has enough vmem to hold the alloc.
	if (ret == __KNULL) {
		return __KNULL;
	};

	// Get at least one frame to back the allocation before returning.

	// FIXME: Also note that this cast is risky.
	for (; (nFound == 0) && (nFound <= static_cast<sarch_t>( nPages )); )
	{
#if __SCALING__ >= SCALING_CC_NUMA
		nFound = numaTrib.configuredGetFrames(
			&cpuTrib.getCurrentCpuStream()->currentTask->numaConfig,
			nPages, &paddr);
#else
		nFound = numaTrib.fragmentedGetFrames(nPages, &paddr);
#endif
		if (nFound > 0)
		{
			if (nFound == static_cast<sarch_t>( nPages ))
			{
				err = allocTable.addEntry(
					ret, nPages, ALLOCTABLE_TYPE_DATA,
					ALLOCTABLE_FLAGS_CONTIG);
			}
			else
			{
				err = allocTable.addEntry(
					ret, nPages, ALLOCTABLE_TYPE_DATA,
					ALLOCTABLE_FLAGS_FRAGMENTED);
			};

			/* Test to see whether or not the allocation got
			 * recorded. If not, then there is an imminent memory
			 * leak. In such a case, we simply refuse to go through
			 * with the allocation and return a NULL pointer to the
			 * application, and free any memory we may have gained
			 * during this function's scope.
			 **/
			if (err != ERROR_SUCCESS)
			{
				/* This is a very interesting response. Instead
				 * of panicking, we simply release the memory
				 * involved.
				 *
				 * The kernel then returns a value which
				 * indicates that "this allocation failed",
				 * whereas in reality, the allocation itself
				 * was successful. We just weren't able to set
				 * up monitoring on it.
				 **/
				// FIXME: Place a warning output here.
				goto releasePmem;
			};

			// FIXME: Later on, change the way you determine this.
			if (memoryStreamC::id == __KPROCESSID)
			{
				nMapped = walkerPageRanger::__kdataMap(
					&vaddrSpaceStream.vaddrSpace,
					ret, paddr, nFound);

			}
			else
			{
				nMapped = walkerPageRanger::dataMap(
					&vaddrSpaceStream.vaddrSpace,
					ret, paddr, nFound);

			};

			if (nMapped < nFound)
			{
				/* If this branch is taken, it means that when
				 * the kernel tried to walk the page tables and
				 * map the allocation into the process's
				 * address space, it failed to map all of the
				 * specified range.
				 *
				 * The only reason for this to happen would be
				 * that at some level of the paging hierarchy,
				 * the kernel needed to allocate a new page
				 * table, but there was no physical memory left
				 * to fulfil that need.
				 *
				 * To be honest, if this branch is taken, there
				 * is very little we can do other than place a
				 * call to the scheduler to dormant the thread,
				 * with the condition that it should be
				 * awakened when the page table cache has free
				 * frames.
				 *
				 *	FIXME: When the scheduler is done, get
				 * this in order.
				 **/
				panic(ERROR_GENERAL, mmStr[4]);
			};
			break;
		};
	};
	/* We should only reach here when some physical memory has been bound
	 * to the allocation.
	 *
	 * At this point, we have the following circumstances: The allocation
	 * is added to the allocation record table, allocTable. The kernel has
	 * also allocated room in the process's virtual address space for the
	 * allocation. This room may or not not fully backed by physical mem.
	 *
	 * So we have an allocation which may or may not be fully backed, and
	 * thus N pages which are backed, and N pages which aren't. Calculate
	 * how many aren't and map them all with walkerPageRangerC::fakeMap().
	 **/
	if (memoryStreamC::id == __KPROCESSID)
	{
		nMapped = walkerPageRanger::__kfakeMap(
			&vaddrSpaceStream.vaddrSpace,
			reinterpret_cast<void *>(
				reinterpret_cast<uarch_t>( ret )
					+ (nFound * PAGING_BASE_SIZE) ),
			nPages - nFound,
			// This is a data allocation. Should be RW.
			PAGEATTRIB_WRITE);
	}
	else
	{
		nMapped = walkerPageRanger::fakeMap(
			&vaddrSpaceStream.vaddrSpace,
			reinterpret_cast<void *>(
				reinterpret_cast<uarch_t>( ret )
					+ (nFound * PAGING_BASE_SIZE) ),
			nPages - nFound,
			// Again, it's a data allocation. Should be RW.
			PAGEATTRIB_WRITE);
	};

	if (nMapped < static_cast<sarch_t>( nPages ) - nFound) {
		panic(ERROR_GENERAL, mmStr[5]);
	};

	return ret;

releasePmem:
	numaTrib.releaseFrames(paddr, nFound);
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
	if ((nPages == 1) && (allocCache.push(nPages, vaddr) == ERROR_SUCCESS))
	{
		__kdebug.printf(NOTICE"Memory Stream %X: memFree(%p): free to "
			"cache.\n", id, vaddr);

		walkerPageRanger::setAttributes(
			&vaddrSpaceStream.vaddrSpace,
			vaddr, nPages, WPRANGER_OP_CLEAR_PRESENT, 0);

		return;
	};

	/**	Optimization:
	 * If the allocation has the contig flag set, then we can free the
	 * whole range in one go. Else, we'll have to just go one by one.
	 **/
	if (__KFLAG_TEST(flags, ALLOCTABLE_FLAGS_CONTIG))
	{
		status = walkerPageRanger::unmap(
			&vaddrSpaceStream.vaddrSpace,
			vaddr, &paddr, nPages, &unmapFlags);

		if (status == WPRANGER_STATUS_BACKED) {
			numaTrib.releaseFrames(paddr, nPages);
		};
	}
	else
	{
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
	};
	// Free the virtual memory.
	vaddrSpaceStream.releasePages(vaddr, nPages);
	// Remove the entry from the process's Alloc Table.
	allocTable.removeEntry(vaddr);	
}

