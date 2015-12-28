
#include <scaling.h>
#include <chipset/memory.h>
#include <__kstdlib/__kmath.h>
#include <__kstdlib/__kclib/string.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/numaMemoryBank.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <__kthreads/main.h>
#include <__kthreads/__kcpuPowerOn.h>


/**	NOTES:
 * Because of the design of NUMA based memory allocation, there is some added
 * complication in the process of migrating threads from one NUMA bank to
 * another, when the scheduler is trying to do load balancing on a non-cache-
 * coherent NUMA board.
 *
 * In any non-CC NUMA setup, not every memory bank may be reachable from every
 * CPU. The CPUs on bank 0 may not be able to reach memory bank 4, but the CPUs
 * on bank 3 may be able to. Now, if a process is originally scheduled to run on
 * CPUs from bank 3, and the kernel happens to allocate memory for it from
 * memory bank 4, what will happen if the load balancer migrates threads from
 * that process to CPU bank 0?
 *
 * It would mean that all memory that has been allocated to that process that
 * comes from memory bank 4 will be unreadable. Therefore, when the scheduler's
 * load balancing idle thread is attempting to migrate threads across possibly
 * non-cache-coherent NUMA domains, it should examine the two domains in
 * question to ensure that all memory that is reachable from the original bank
 * is also reachable in the proposed new bank. Migrating threads to new CPUs in
 * the same bank is not a problem.
 *
 * Naturally, this does not only affect memory allocations explicitly called for
 * by the requesting process, but also has an impact on the ability of CPUs
 * to walk page tables when a thread is migrated to a CPU which cannot read
 * the RAM on a bank where page tables for that process may have been allocated.
 **/

// The __kspace allocatable memory range's containing memory bank.
static NumaMemoryBank			__kspaceMemoryBank(
	CHIPSET_NUMA___KSPACE_BANKID);

// The pointer node on the __kspace bank that points to the __kspace mem range.
static NumaMemoryBank::sRangePtr	__kspaceRangePtrMem;
// The __kspace mem range, which has a frame cache and a BMP.
static NumaMemoryRange			__kspaceMemoryRange(
	CHIPSET_MEMORY___KSPACE_BASE,
	CHIPSET_MEMORY___KSPACE_SIZE);

// Reserve space for the __kspaceBmp within the kernel image:
static uarch_t		__kspaceInitMem[
	__KMATH_NELEMENTS(
		(CHIPSET_MEMORY___KSPACE_SIZE / PAGING_BASE_SIZE),
		(sizeof(uarch_t) * __BITS_PER_BYTE__))];

error_t MemoryTrib::__kspaceInitialize(void)
{
	error_t		ret;

	/**	EXPLANATION:
	 * This function literally assembles the boot time physical memory
	 * allocator (__kspace) for the chipset's __kspace region, by simply
	 * putting together pre-allocated and initialized memory management
	 * structures.
	 *
	 * The comments below should suffice. We essentially set up an array
	 * of NUMA banks with memory on them, and place __kspace on it as a fake
	 * bank. Then we add the __kspace memory region to the bank.
	 **/
#if __SCALING__ < SCALING_CC_NUMA
	defaultMemoryBank.rsrc = CHIPSET_NUMA___KSPACE_BANKID;
#else
	bspCpu.taskStream.getCurrentThread()->defaultMemoryBank.rsrc =
		CHIPSET_NUMA___KSPACE_BANKID;
#endif

	// Next give it the pre-allocated __kspace memory bank.
	ret = createBank(CHIPSET_NUMA___KSPACE_BANKID, &__kspaceMemoryBank);
	if (ret != ERROR_SUCCESS) {
		return ret;
	};

	/* Next give it the pre-allocated __kspace memory range object.
	 *
	 * NOTE: NumaMemoryBank already calls initialize() on a
	 * NumaMemoryRange object when it's adding it to its internal list.
	 *
	 * Do not explicitly call initialize() on the NumaMemoryRange object
	 * before giving it to the __kspace NumaMemoryBank object.
	 **/
	ret = getBank(CHIPSET_NUMA___KSPACE_BANKID)
		->__kspaceAddMemoryRange(
			&__kspaceRangePtrMem,
			&__kspaceMemoryRange,
			__kspaceInitMem);

	return ret;
}

error_t MemoryTrib::createBank(numaBankId_t id, NumaMemoryBank *preAllocated)
{
	error_t			ret;
	NumaMemoryBank		*nmb;

	CHECK_AND_RESIZE_BMP(
		&availableBanks, id, &ret, "createBank", "availableBanks");

	if (ret != ERROR_SUCCESS) { return ret; };

	// Note the MEMALLOC_NO_FAKEMAP flag: MM code/data must never pgfault.
	if (preAllocated == NULL)
	{
		nmb = new (processTrib.__kgetStream()->memoryStream.memAlloc(
			PAGING_BYTES_TO_PAGES(sizeof(NumaMemoryBank)),
			MEMALLOC_NO_FAKEMAP))
				NumaMemoryBank(id);
	}
	else {
		nmb = new (preAllocated) NumaMemoryBank(id);
	};

	if (nmb == NULL) {
		return ERROR_MEMORY_NOMEM;
	};

	ret = nmb->initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	ret = memoryBanks.addItem(id, nmb);
	if (ret != ERROR_SUCCESS)
	{
		if (preAllocated != NULL) {
			processTrib.__kgetStream()->memoryStream.memFree(nmb);
		};
	};

	availableBanks.setSingle(id);
	nBanks++;
	return ret;
}

void MemoryTrib::destroyBank(numaBankId_t id)
{
	NumaMemoryBank		*nmb;

	availableBanks.unsetSingle(id);
	nmb = getBank(id);
	memoryBanks.removeItem(id);
	nBanks--;
	if (id != CHIPSET_NUMA___KSPACE_BANKID && nmb != NULL) {
		processTrib.__kgetStream()->memoryStream.memFree(nmb);
	};
}

void MemoryTrib::releaseFrames(paddr_t paddr, uarch_t nFrames)
{
	NumaMemoryBank			*currBank;
	HardwareIdList::Iterator	it;

	/**	EXPLANATION:
	 * Here we have the result of a trade-off. For a reduction in the size
	 * of each entry in AllocTable and better design overall, and smaller
	 * memory footprint, we traded off speed on memory frees.
	 *
	 * The kernel now has to decipher which bank a range of pmem belongs to
	 * before it can be freed.
	 **/
#if __SCALING__ >= SCALING_CC_NUMA
	for (it = memoryBanks.begin(); it != memoryBanks.end(); ++it)
	{
		currBank = (NumaMemoryBank *)*it;

		if (currBank->identifyPaddr(paddr))
		{
			currBank->releaseFrames(paddr, nFrames);
			return;
		};
	};
	/* Couldn't find a suitable bank. Probably the memory was hot swapped,
	 * or there's corruption in the memory manager somewhere.
	 **/
	printf(WARNING MEMTRIB"releaseFrames(0x%P, %d): pmem leak.\n",
		&paddr, nFrames);
#else
	currBank = getBank(defaultMemoryBank.rsrc);
	if (currBank) {
		currBank->releaseFrames(paddr, nFrames);
	}
	else
	{
		printf(FATAL MEMTRIB"releaseFrames(0x%P, %d): Attempted to "
			"free to non-existent mem bank %d.\n",
			paddr, nFrames, defaultAffinity.def.rsrc);
	};
#endif

}

#if __SCALING__ < SCALING_CC_NUMA
error_t MemoryTrib::contiguousGetFrames(uarch_t nPages, paddr_t *paddr, ubit32)
{
	error_t			ret;
	NumaMemoryBank		*currBank;

	/* Allocate from the current default bank, which is either __kspace or
	 * shared-bank, depending on which stage of memory management
	 * initialization the kernel is currently at.
	 **/
	currBank = getBank(defaultMemoryBank.rsrc);
	if (currBank != NULL)
	{
		ret = currBank->contiguousGetFrames(nPages, paddr);
		if (ret == ERROR_SUCCESS) {
			return ret;
		};
	};

	// If we reached here, then we've failed completely.
	return ERROR_MEMORY_NOMEM_PHYSICAL;
}

error_t MemoryTrib::fragmentedGetFrames(uarch_t nPages, paddr_t *paddr, ubit32)
{
	error_t			ret;
	NumaMemoryBank		*currBank;

	/* Allocate from the current default bank, which is either __kspace or
	 * shared-bank, depending on which stage of memory management
	 * initialization the kernel is currently at.
	 **/
	currBank = getBank(defaultMemoryBank.rsrc);
	if (currBank != NULL)
	{
		ret = currBank->fragmentedGetFrames(nPages, paddr);
		if (ret > 0) {
			return ret;
		};
	};

	// If we reached here, then we've failed completely.
	return ERROR_MEMORY_NOMEM_PHYSICAL;
}
#endif /* if __SCALING__ < SCALING_CC_NUMA */

#if __SCALING__ >= SCALING_CC_NUMA
error_t MemoryTrib::fragmentedGetFrames(uarch_t nPages, paddr_t *paddr, ubit32)
{
	HardwareIdList::Iterator	currIt;
	NumaMemoryBank			*currBank;
	error_t				ret;
	uarch_t				rwFlags;
	Thread				*thread;

	thread = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	// Get the calling thread's default memory bank.
	thread->defaultMemoryBank.lock.readAcquire(&rwFlags);
	currIt.cursor = thread->defaultMemoryBank.rsrc;
	thread->defaultMemoryBank.lock.readRelease(rwFlags);

	currBank = getBank(currIt.cursor);
	if (currBank != NULL)
	{
		ret = currBank->fragmentedGetFrames(nPages, paddr);
		if (ret > 0) {
			return ret;
		};
	};

	/**	FIXME: Re-arrange this loop.
	 * Make it so that the kernel checks each reachable memory bank from the
	 * thread's current CPU bank's perspective. For each reachable memory
	 * bank, check to see if memory is available, and if so, set that bank
	 * to be the thread's new default.
	 **/

	// Allocation from the default bank failed. Find another default bank.
	currIt = memoryBanks.begin();
	for (; currIt != memoryBanks.end(); ++currIt)
	{
		currBank = (NumaMemoryBank *)*currIt;

		ret = currBank->fragmentedGetFrames(nPages, paddr);
		if (ret > 0)
		{
			thread->defaultMemoryBank.lock.writeAcquire();
			thread->defaultMemoryBank.rsrc = currIt.cursor;
			thread->defaultMemoryBank.lock.writeRelease();
			return ret;
		};
	};

	/* If we reach here then none of the thread's configured banks have any
	 * physical mem left on them.
	 **/
	return 0;
}
#endif /* if __SCALING__ >= SCALING_CC_NUMA */

void MemoryTrib::mapRangeUsed(paddr_t baseAddr, uarch_t nPages)
{
	NumaMemoryBank	*currBank;

#if __SCALING__ >= SCALING_CC_NUMA
	HardwareIdList::Iterator	it = memoryBanks.begin();
	for (; it != memoryBanks.end(); ++it)
	{
		currBank = (NumaMemoryBank *)*it;

		/* We can most likely afford this small speed bump since ranges
		 * of physical RAM are not often mapped or unmapped as used at
		 * runtime. This generally only happens when the kernel is made
		 * aware of an MMIO range.
		 **/
		currBank->mapMemUsed(baseAddr, nPages);
	};
#else
	currBank = getBank(defaultMemoryBank.rsrc);
	if (currBank != NULL) {
		currBank->mapMemUsed(baseAddr, nPages);
	}
	else
	{
		printf(ERROR MEMTRIB"mapRangeUsed(0x%P, %d): attempt to "
			"mark on non-existent bank %d.\n",
			defaultAffinity.def.rsrc);
	};
#endif
}

void MemoryTrib::mapRangeUnused(paddr_t baseAddr, uarch_t nPages)
{
	NumaMemoryBank	*currBank;

#if __SCALING__ >= SCALING_CC_NUMA
	HardwareIdList::Iterator		it = memoryBanks.begin();
	for (; it != memoryBanks.end(); ++it)
	{
		currBank = (NumaMemoryBank *)*it;

		/* We can most likely afford this small speed bump since ranges
		 * of physical RAM are not often mapped or unmapped as used at
		 * runtime. This generally only happens when the kernel is made
		 * aware of an MMIO range.
		 **/
		currBank->mapMemUnused(baseAddr, nPages);
	};
#else
	currBank = getBank(defaultMemoryBank.rsrc);
	if (currBank != NULL) {
		currBank->mapMemUnused(baseAddr, nPages);
	}
	else
	{
		printf(ERROR MEMTRIB"mapRangeUnused(0x%P, %d): attempt to "
			"mark on non-existent bank %d.\n",
			defaultAffinity.def.rsrc);
	};
#endif
}

