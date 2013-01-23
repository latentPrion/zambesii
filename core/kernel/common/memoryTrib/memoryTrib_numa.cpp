
#include <scaling.h>
#include <chipset/memory.h>
#include <__kstdlib/__kclib/string.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/numaMemoryBank.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <__kthreads/__korientation.h>
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
static numaMemoryBankC			__kspaceMemoryBank(
	CHIPSET_MEMORY_NUMA___KSPACE_BANKID);

// The pointer node on the __kspace bank that points to the __kspace mem range.
static numaMemoryBankC::rangePtrS	__kspaceRangePtrMem;
// The __kspace mem range, which has a frame cache and a BMP.
static numaMemoryRangeC			__kspaceMemoryRange(
	CHIPSET_MEMORY___KSPACE_BASE,
	CHIPSET_MEMORY___KSPACE_SIZE);

error_t memoryTribC::__kspaceInit(void)
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
	defaultMemoryBank.rsrc = CHIPSET_MEMORY_NUMA___KSPACE_BANKID;
#else
	__korientationThread.defaultMemoryBank.rsrc =
		CHIPSET_MEMORY_NUMA___KSPACE_BANKID;

	__kcpuPowerOnThread.defaultMemoryBank.rsrc =
		CHIPSET_MEMORY_NUMA___KSPACE_BANKID;
#endif

	// First give the list class pre-allocated memory to use for its array.
	memset(
		initialMemoryBankArray, 0,
		sizeof(hardwareIdListC::arrayNodeS)
			* (CHIPSET_MEMORY_NUMA___KSPACE_BANKID + 1));

	memoryBanks.__kspaceSetState(
		CHIPSET_MEMORY_NUMA___KSPACE_BANKID,
		static_cast<void *>( initialMemoryBankArray ));

	// Next give it the pre-allocated __kspace memory bank.
	ret = memoryBanks.addItem(
		CHIPSET_MEMORY_NUMA___KSPACE_BANKID, &__kspaceMemoryBank);

	if (ret != ERROR_SUCCESS) {
		return ret;
	};

	nBanks = 1;

	/* Next give it the pre-allocated __kspace memory range object.
	 *
	 * NOTE: numaMemoryBankC already calls initialize() on a
	 * numaMemoryRangeC object when it's adding it to its internal list.
	 *
	 * Do not explicitly call initialize() on the numaMemoryRangeC object
	 * before giving it to the __kspace numaMemoryBankC object.
	 **/
	ret = getBank(CHIPSET_MEMORY_NUMA___KSPACE_BANKID)
		->__kspaceAddMemoryRange(
			&__kspaceRangePtrMem,
			&__kspaceMemoryRange,
			__kspaceInitMem);

	return ret;
}

/* Args:
 * __pb = pointer to the bitmapC object to be checked.
 * __n = the bit number which the bitmap should be able to hold. For example,
 *	 if the bit number is 32, then the BMP will be checked for 33 bit
 *	 capacity or higher, since 0-31 = 32 bits, and bit 32 would be the 33rd
 *	 bit.
 * __ret = pointer to variable to return the error code from the operation in.
 * __fn = The name of the function this macro was called inside.
 * __bn = the human recognizible name of the bitmapC instance being checked.
 *
 * The latter two are used to print out a more useful error message should an
 * error occur.
 **/
#define CHECK_AND_RESIZE_BMP(__pb,__n,__ret,__fn,__bn)			\
	do { \
	*(__ret) = ERROR_SUCCESS; \
	if ((__n) > (signed)(__pb)->getNBits() - 1) \
	{ \
		*(__ret) = (__pb)->resizeTo((__n) + 1); \
		if (*(__ret) != ERROR_SUCCESS) \
		{ \
			__kprintf(ERROR MEMTRIB"%s: resize failed on %s with " \
				"required capacity = %d.\n", \
				__fn, __bn, __n); \
		}; \
	}; \
	} while (0);
	
error_t memoryTribC::createBank(numaBankId_t id)
{
	error_t			ret;
	numaMemoryBankC		*nmb;

	CHECK_AND_RESIZE_BMP(
		&availableBanks, id, &ret, "createBank", "availableBanks");

	if (ret != ERROR_SUCCESS) { return ret; };

	// Note the MEMALLOC_NO_FAKEMAP flag: MM code/data should never pgfault.
	nmb = new (processTrib.__kprocess.memoryStream.memAlloc(
		PAGING_BYTES_TO_PAGES(sizeof(numaMemoryBankC)),
		MEMALLOC_NO_FAKEMAP))
			numaMemoryBankC(id);

	if (nmb == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};

	ret = memoryBanks.addItem(id, nmb);
	if (ret != ERROR_SUCCESS) {
		processTrib.__kprocess.memoryStream.memFree(nmb);
	};

	availableBanks.setSingle(id);
	return ret;
}

void memoryTribC::destroyBank(numaBankId_t id)
{
	numaMemoryBankC		*nmb;

	availableBanks.unsetSingle(id);
	nmb = getBank(id);
	memoryBanks.removeItem(id);
	if (nmb != __KNULL) {
		processTrib.__kprocess.memoryStream.memFree(nmb);
	};
}

void memoryTribC::releaseFrames(paddr_t paddr, uarch_t nFrames)
{
	numaMemoryBankC	*currBank;
	numaBankId_t	cur;

	/**	EXPLANATION:
	 * Here we have the result of a trade-off. For a reduction in the size
	 * of each entry in allocTableC and better design overall, and smaller
	 * memory footprint, we traded off speed on memory frees.
	 *
	 * The kernel now has to decipher which bank a range of pmem belongs to
	 * before it can be freed.
	 **/
#if __SCALING__ >= SCALING_CC_NUMA
	cur = memoryBanks.prepareForLoop();
	currBank = (numaMemoryBankC *)memoryBanks.getLoopItem(&cur);

	for (; currBank != __KNULL;
		currBank = (numaMemoryBankC *)memoryBanks.getLoopItem(&cur))
	{
		if (currBank->identifyPaddr(paddr))
		{
			currBank->releaseFrames(paddr, nFrames);
			return;
		};
	};
	/* Couldn't find a suitable bank. Probably the memory was hot swapped,
	 * or there's corruption in the memory manager somewhere.
	 **/
	__kprintf(WARNING MEMTRIB"releaseFrames(0x%P, %d): pmem leak.\n",
		paddr, nFrames);
#else
	currBank = getBank(defaultMemoryBank.rsrc);
	if (currBank) {
		currBank->releaseFrames(paddr, nFrames);
	}
	else
	{
		__kprintf(FATAL MEMTRIB"releaseFrames(0x%P, %d): Attempted to "
			"free to non-existent mem bank %d.\n",
			paddr, nFrames, defaultAffinity.def.rsrc);
	};
#endif

}

#if __SCALING__ < SCALING_CC_NUMA
error_t memoryTribC::contiguousGetFrames(uarch_t nPages, paddr_t *paddr)
{
	error_t			ret;
	numaMemoryBankC		*currBank;

	/* Allocate from the current default bank, which is either __kspace or
	 * shared-bank, depending on which stage of memory management
	 * initialization the kernel is currently at.
	 **/
	currBank = getBank(defaultMemoryBank.rsrc);
	if (currBank != __KNULL)
	{
		ret = currBank->contiguousGetFrames(nPages, paddr);
		if (ret == ERROR_SUCCESS) {
			return ret;
		};
	};

	// If we reached here, then we've failed completely.
	return ERROR_MEMORY_NOMEM_PHYSICAL;
}

error_t memoryTribC::fragmentedGetFrames(uarch_t nPages, paddr_t *paddr)
{
	error_t			ret;
	numaMemoryBankC		*currBank;

	/* Allocate from the current default bank, which is either __kspace or
	 * shared-bank, depending on which stage of memory management
	 * initialization the kernel is currently at.
	 **/
	currBank = getBank(defaultMemoryBank.rsrc);
	if (currBank != __KNULL)
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
error_t memoryTribC::fragmentedGetFrames(uarch_t nPages, paddr_t *paddr)
{
	numaBankId_t		def, cur;
	numaMemoryBankC		*currBank;
	error_t			ret;
	uarch_t			rwFlags;
	taskC			*currTask;

	// Get the calling thread's default memory bank.
	currTask = cpuTrib.getCurrentCpuStream()->taskStream.currentTask;

	currTask->defaultMemoryBank.lock.readAcquire(&rwFlags);
	def = currTask->defaultMemoryBank.rsrc;
	currTask->defaultMemoryBank.lock.readRelease(rwFlags);

	currBank = getBank(def);
	if (currBank != __KNULL)
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
	def = cur = memoryBanks.prepareForLoop();
	currBank = (numaMemoryBankC *)memoryBanks.getLoopItem(&def);

	for (; currBank != __KNULL;
		currBank = (numaMemoryBankC *)memoryBanks.getLoopItem(&def))
	{
		ret = currBank->fragmentedGetFrames(nPages, paddr);
		if (ret > 0)
		{
			currTask->defaultMemoryBank.lock.writeAcquire();
			currTask->defaultMemoryBank.rsrc = cur;
			currTask->defaultMemoryBank.lock.writeRelease();
			return ret;
		};
		cur = def;
	};

	/* If we reach here then none of the thread's configured banks have any
	 * physical mem left on them.
	 **/
	return 0;
}
#endif /* if __SCALING__ >= SCALING_CC_NUMA */

void memoryTribC::mapRangeUsed(paddr_t baseAddr, uarch_t nPages)
{
#if __SCALING__ >= SCALING_CC_NUMA
	numaBankId_t	cur;
#endif
	numaMemoryBankC	*currBank;

#if __SCALING__ >= SCALING_CC_NUMA
	cur = memoryBanks.prepareForLoop();
	currBank = (numaMemoryBankC *)memoryBanks.getLoopItem(&cur);

	for (; currBank != __KNULL;
		currBank = (numaMemoryBankC *)memoryBanks.getLoopItem(&cur))
	{
		/* We can most likely afford this small speed bump since ranges
		 * of physical RAM are not often mapped or unmapped as used at
		 * runtime. This generally only happens when the kernel is made
		 * aware of an MMIO range.
		 **/
		currBank->mapMemUsed(baseAddr, nPages);
	};
#else
	currBank = getBank(defaultMemoryBank.rsrc);
	if (currBank != __KNULL) {
		currBank->mapMemUsed(baseAddr, nPages);
	}
	else
	{
		__kprintf(ERROR MEMTRIB"mapRangeUsed(0x%P, %d): attempt to "
			"mark on non-existent bank %d.\n",
			defaultAffinity.def.rsrc);
	};
#endif
}

void memoryTribC::mapRangeUnused(paddr_t baseAddr, uarch_t nPages)
{
#if __SCALING__ >= SCALING_CC_NUMA
	numaBankId_t	cur;
#endif
	numaMemoryBankC	*currBank;


#if __SCALING__ >= SCALING_CC_NUMA
	cur = memoryBanks.prepareForLoop();
	currBank = (numaMemoryBankC *)memoryBanks.getLoopItem(&cur);
	
	for (; currBank != __KNULL;
		currBank = (numaMemoryBankC *)memoryBanks.getLoopItem(&cur))
	{
		/* We can most likely afford this small speed bump since ranges
		 * of physical RAM are not often mapped or unmapped as used at
		 * runtime. This generally only happens when the kernel is made
		 * aware of an MMIO range.
		 **/
		currBank->mapMemUnused(baseAddr, nPages);
	};
#else
	currBank = getBank(defaultMemoryBank.rsrc);
	if (currBank != __KNULL) {
		currBank->mapMemUnused(baseAddr, nPages);
	}
	else
	{
		__kprintf(ERROR MEMTRIB"mapRangeUnused(0x%P, %d): attempt to "
			"mark on non-existent bank %d.\n",
			defaultAffinity.def.rsrc);
	};
#endif
}

