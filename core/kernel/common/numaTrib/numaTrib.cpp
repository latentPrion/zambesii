
#include <scaling.h>
#include <arch/paging.h>
#include <chipset/__kmemory.h>
#include <chipset/numaMap.h>
#include <chipset/memoryMap.h>
#include <chipset/memoryConfig.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kstdlib/__kclib/string.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/numaTrib/numaTrib.h>

/**	EXPLANATION:
 * Setting up the NUMA Tributary is a bit more complicated than I'd imagined.
 * There are several things to consider:
 *	1. The NUMA Stream pointer array. If we do not pre-allocate memory
 *	   in the kernel image for it, then it will remain a pointer, and not
 *	   a pointer to a block of memory partitioned into pointers.
 *	2. Then we have to ensure that as soon as dynamic pages are available
 *	   we re-allocate the array so that the kernel can use it. If we don't
 *	   do that, then when the kernel needs to re-size the array, it will
 *	   of course, free it, and this will cause the kernel to get trampled.
 *	   There is a way to avoid the trampling, and that is to allocate the
 *	   pages for the NUMA stream array from the kernel Memory Stream. This
 *	   way our allocations are recorded and any false frees will be ignored.
 *	3. To initialize the actual default bank, first we need to make sure
 *	   that there is a pre-allocated NUMA stream in the kernel image,
 *	   completely uninitialized (constructed with the default constructor).
 *	4. Next we have to set the numaStreams resource to point to the
 *	   pre-allocated array.
 *	5. After this we have to point the first pointer in the array to the
 *	   address of the __kspace bank.
 *	6. Then we need to call initialize() on the new first bank.
 *	7. When this returns, then we know that the __kspace BMP is now ready
 *	   to be used to allocate pages.
 **/
static numaStreamC		*initNumaStreamArray[
	PAGING_PAGES_TO_BYTES(1) / sizeof(void *)];

static numaStreamC		__kspaceNumaStream;


numaTribC::numaTribC(void)
{
}

error_t numaTribC::initialize(void)
{
	error_t		ret;

	initNumaStreamArray[0] = &__kspaceNumaStream;
	numaStreams.rsrc = initNumaStreamArray;

	streamArrayNPages = 1;
	nStreams = 1;

	// Call initialize() on the __kspace NUMA Stream, give it an ID of 0.
	ret = numaStreams.rsrc[0]->initialize(
		0,
		CHIPSET_MEMORY___KSPACE_BASE,
		CHIPSET_MEMORY___KSPACE_SIZE,
		__kspaceInitMem);

	if (ret != ERROR_SUCCESS) {
		return ret;
	};

	/**	EXPLANATION:
	 * Right now, if we're still here, then it means that the kernel's
	 * __kspace bank has just been successfully initalize()d. We now have a
	 * small bank of physical RAM from which we can allocate (__kspace).
	 *
	 * However, if we call any of the non-configured getFrames() functions,
	 * we'll end up having the kernel search for RAM on banks using the
	 * default config's internal BMP. This would be disastrous since the
	 * object is not yet initialized.
	 *
	 * To work around this, set the default config's default bank to be
	 * __kspace. Also, the bitmapC class's constructor sets its internal
	 * 'nBits' member to 0 on construct. So if call to check its bits is
	 * made while it's not yet initialized, it will return FALSE for if
	 * that bit is set.
	 **/
#if __SCALING__ >= SCALING_CC_NUMA
	// Make sure that the NUMA Trib will only allocate from __kspace.
	defaultConfig.def.rsrc = 0;
#endif

	// We have now, barring the uninitialized default config, full PMM.
	return ERROR_SUCCESS;
}

numaTribC::~numaTribC(void)
{
	if (numaStreams.rsrc != __KNULL)
	{
		for (; nStreams > 0; nStreams--)
		{
			if (numaStreams.rsrc[nStreams] != __KNULL) {
				delete numaStreams.rsrc[nStreams];
			};
		};
		/* FIXME: Think this over and see whether it should be kernel
		 * stream allocated. Read the comments about (point 2) to see
		 * what you mean.
		 **/
		memoryTrib.rawMemFree(numaStreams.rsrc, streamArrayNPages);
	};
}

#if __SCALING__ >= SCALING_CC_NUMA
numaStreamC *numaTribC::getStream(numaBankId_t bankId)
{
	numaStreamC		*ret;
	uarch_t			rwFlags;

	numaStreams.lock.readAcquire(&rwFlags);

	ret = numaStreams.rsrc[bankId];

	numaStreams.lock.readRelease(rwFlags);
	return ret;
}
#endif

void numaTribC::releaseFrames(paddr_t paddr, uarch_t nFrames)
{
	uarch_t		rwFlags;
	numaStreamC	*currStream;

	/**	EXPLANATION:
	 * Here we have the result of a trade-off. For a reduction in the size
	 * of each entry in allocTableC and better design overall, and smaller
	 * memory footprint, we traded off speed on memory frees.
	 *
	 * The kernel now has to decipher which bank a range of pmem belongs to
	 * before it can be freed.
	 **/
#if __SCALING__ >= SCALING_CC_NUMA
	for (uarch_t i=0; i<nStreams; i++)
	{
		numaStreams.lock.readAcquire(&rwFlags);
		currStream = numaStreams.rsrc[i];
		numaStreams.lock.readRelease(rwFlags);

		// Ensure we're not trying to free to hot-swapped out, etc RAM.
		if (currStream != __KNULL)
		{
			// TODO: There must be a way to optimize this.
			if ((paddr > currStream->memoryBank.baseAddr) &&
				(paddr < (currStream->memoryBank.baseAddr +
					currStream->memoryBank.size)))
			{
				currStream->memoryBank.releaseFrames(
					paddr, nFrames);
				return;
			};
		};
	};
	/* Couldn't find a suitable bank. Probably the memory was hot swapped,
	 * or there's corruption in the memory manager somewhere.
	 **/
	// FIXME: Place a log warning here.
#else
	numaStreams.rsrc[0]->memoryBank.releaseFrames(paddr, nFrames);
#endif

}

error_t numaTribC::contiguousGetFrames(uarch_t nPages, paddr_t *paddr)
{
	numaBankId_t		def;
	uarch_t			rwFlags;
	numaStreamC		*currStream;
	error_t			ret;

#if __SCALING__ >= SCALING_CC_NUMA
	// Get the default bank's Id.
	defaultConfig.def.lock.readAcquire(&rwFlags);

	def = defaultConfig.def.rsrc;

	defaultConfig.def.lock.readRelease(rwFlags);
#else
	// For a non-NUMA build, the kernel just fakes a bank with id=0.
	def = 0;
#endif

	// Allocate from the default bank.
	currStream = getStream(def);
	// FIXME: Decide what to do here for an non-NUMA build.
	if (currStream != __KNULL)
	{
		ret = currStream->memoryBank.contiguousGetFrames(nPages, paddr);
		if (ret == ERROR_SUCCESS) {
			return ret;
		};
	};

#if __SCALING__ >= SCALING_CC_NUMA
	/* If we're still here then there was no memory on the default bank. We
	 * have to now determine which bank would have memory, and set that to
	 * be the new default bank for raw contiguous allocations.
	 **/
	for (ubit32 i=0; i<nStreams; i++)
	{
		currStream = getStream(i);
		if (currStream != __KNULL)
		{
			// Attempt to allocate from the current stream.
			ret = currStream->memoryBank.contiguousGetFrames(
				nPages, paddr);

			if (ret == ERROR_SUCCESS)
			{
				// Set the current stream to be the new default.
				defaultConfig.def.lock.writeAcquire();

				defaultConfig.def.rsrc = i;

				defaultConfig.def.lock.writeRelease();
				// Return. We got memory off the current bank.
				return ret;
			};
		};
	};
#endif
	// If we reached here, then we've failed completely.
	return ERROR_MEMORY_NOMEM_PHYSICAL;
}

error_t numaTribC::fragmentedGetFrames(uarch_t nPages, paddr_t *paddr)
{
	error_t			ret=0;
	numaBankId_t		def;
	uarch_t			rwFlags;
	numaStreamC		*currStream;

#if __SCALING__ >= SCALING_CC_NUMA
	defaultConfig.def.lock.readAcquire(&rwFlags);

	def = defaultConfig.def.rsrc;

	defaultConfig.def.lock.readRelease(rwFlags);
#else
	def = 0;
#endif

	//Now allocate from the default bank.
	currStream = getStream(def);
	if (currStream != __KNULL)
	{
		ret = currStream->memoryBank.fragmentedGetFrames(
			nPages, paddr);

		if (ret > 0) {
			return ret;
		};
	};

#if __SCALING__ >= SCALING_CC_NUMA
	/* If we're still here then we failed at allocating from the default
	 * bank. Search each other bank, and get frames from one of them.
	 **/
	for (ubit32 i=0; i<nStreams; i++)
	{
		currStream = getStream(i);
		if (currStream != __KNULL)
		{
			ret = currStream->memoryBank.fragmentedGetFrames(
				nPages, paddr);

			if (ret > 0)
			{
				defaultConfig.def.lock.writeAcquire();

				defaultConfig.def.rsrc = i;

				defaultConfig.def.lock.writeRelease();
				return ret;
			};
		};
	};
#endif
	// If we're here, then we've failed.
	return ret;
}

// Preprocess out this whole function on a non-NUMA build.
#if __SCALING__ >= SCALING_CC_NUMA
error_t numaTribC::configuredGetFrames(
	numaConfigS *config, uarch_t nPages, paddr_t *paddr
	)
{
	uarch_t			rwFlags;
	numaBankId_t		def;
	numaStreamC		*currStream;
	error_t			ret;

	// Get the thread's default config.
	config->def.lock.readAcquire(&rwFlags);

	def = config->def.rsrc;

	config->def.lock.readRelease(rwFlags);

	currStream = getStream(def);
	if (currStream != __KNULL)
	{
		ret = currStream->memoryBank.fragmentedGetFrames(
			nPages, paddr);

		if (ret > 0) {
			return ret;
		};
	};

	// Allocation from the default bank failed. Find a another default bank.
	for (ubit32 i=0; i<nStreams; i++)
	{
		// If this bank is part of the thread's NUMA policy:
		if (config->memBanks.testSingle(i))
		{
			currStream = getStream(i);
			if (currStream != __KNULL)
			{
				ret = currStream->memoryBank
					.fragmentedGetFrames(nPages, paddr);

				if (ret > 0)
				{
					config->def.lock.writeAcquire();
					
					config->def.rsrc = i;
					
					config->def.lock.writeRelease();
					return ret;
				};
			};
		};
	};

	/* If we reach here then none of the thread's configured banks has any
	 * physical mem left on it.
	 **/
	return ERROR_MEMORY_NOMEM_PHYSICAL;
}
#endif

void numaTribC::mapRangeUsed(paddr_t baseAddr, uarch_t nPages)
{
	uarch_t		rwFlags;
	numaStreamC	*currStream;

	for (uarch_t i=0; i<nStreams; i++)
	{
		numaStreams.lock.readAcquire(&rwFlags);
		currStream = numaStreams.rsrc[i];
		numaStreams.lock.readRelease(rwFlags);

		/* We can most likely afford this small speed bump since ranges
		 * of physical RAM are not often mapped or unmapped as used at
		 * runtime. This generally only happens when the kernel is made
		 * aware of an MMIO range.
		 **/
		currStream->memoryBank.mapRangeUsed(baseAddr, nPages);
	};
}

void numaTribC::mapRangeUnused(paddr_t baseAddr, uarch_t nPages)
{
	uarch_t		rwFlags;
	numaStreamC	*currStream;

	for (uarch_t i=0; i<nStreams; i++)
	{
		numaStreams.lock.readAcquire(&rwFlags);
		currStream = numaStreams.rsrc[i];
		numaStreams.lock.readRelease(rwFlags);

		currStream->memoryBank.mapRangeUnused(baseAddr, nPages);
	};
}

