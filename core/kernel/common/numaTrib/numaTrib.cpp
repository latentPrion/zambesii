
#include <scaling.h>
#include <arch/paging.h>
#include <chipset/numaMap.h>
#include <chipset/memoryMap.h>
#include <chipset/memoryConfig.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/numaTrib/numaTrib.h>

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

numaTribC::numaTribC(void)
{
	nStreams = 0;
	numaStreams.rsrc = __KNULL;
}

numaTribC::~numaTribC(void)
{
	if (numaStreams.rsrc != __KNULL) {
		memoryTrib.__kspaceMemFree(numaStreams.rsrc, streamArrayNPages);
	};
}

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

error_t numaTribC::initialize(void)
{
	// Report error: function not yet implemented.
	return ERROR_GENERAL;
	// Re-implement later.
}

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

