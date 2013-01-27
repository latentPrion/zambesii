
#include <debug.h>

#include <arch/paging.h>
#include <chipset/memory.h>
#include <__kstdlib/__kmath.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <__kclasses/memReservoir.h>
#include <kernel/common/processTrib/processTrib.h>


memReservoirC::memReservoirC(void)
{
	__kbog = __KNULL;
	bogs.rsrc.ptrs = __KNULL;
	bogs.rsrc.nBogs = 0;
}

error_t memReservoirC::initialize(void)
{
	__kbog = new (processTrib.__kprocess.memoryStream.memAlloc(
		1, MEMALLOC_NO_FAKEMAP))
			memoryBogC(CHIPSET_MEMORY___KBOG_SIZE);

	if (__kbog == __KNULL)
	{
		__kprintf(ERROR RESERVOIR"Unable to allocate a bog for "
			"general kernel use.\n");

		return ERROR_MEMORY_NOMEM;
	};

	__kbog->initialize();

	bogs.rsrc.ptrs = new (processTrib.__kprocess.memoryStream.memAlloc(
		1, MEMALLOC_NO_FAKEMAP))
			memoryBogC*;

	if (bogs.rsrc.ptrs == __KNULL)
	{
		__kprintf(ERROR RESERVOIR"Unable to allocate a page to hold "
			"the array of custom bog allocators.\n");

		return ERROR_MEMORY_NOMEM;
	};

	memset(bogs.rsrc.ptrs, 0, PAGING_BASE_SIZE);

	__kprintf(NOTICE RESERVOIR"initialize(): Done. __kbog v 0x%X, size "
		"0x%X, custom bogs array 0x%X.\n",
		__kbog, __kbog->blockSize, bogs.rsrc.ptrs);

	return ERROR_SUCCESS;
}

memReservoirC::~memReservoirC(void)
{
}

void memReservoirC::dump(void)
{
	
	__kprintf(NOTICE RESERVOIR"Dumping.\n");
	__kprintf(NOTICE RESERVOIR"__kbog v 0x%X, bogs array 0x%X, nBogs %d.\n",
		__kbog, bogs.rsrc.ptrs, bogs.rsrc.nBogs);

	__kprintf(NOTICE RESERVOIR"Dumping __kbog.\n");
	__kbog->dump();

	for (uarch_t i=0; i<bogs.rsrc.nBogs; i++)
	{
		__kprintf(NOTICE RESERVOIR"Dumping bog %d @ v %X.\n",
			i, bogs.rsrc.ptrs[i]);

		bogs.rsrc.ptrs[i]->dump();
	};
}

void *memReservoirC::allocate(uarch_t nBytes, uarch_t flags)
{
	reservoirHeaderS	*ret;
	if (nBytes == 0) {
		return __KNULL;
	};

	nBytes += sizeof(reservoirHeaderS);

	/**	NOTES:
	 * Even though the very fact that the kernel's bog is unavailable is
	 * cause for great alarm, we shouldn't kill all allocations simply
	 * because of that: still try to allocate off of a stream.
	 **/
	if (__kbog == __KNULL) {
		goto tryStream;
	};

	if (nBytes <= __kbog->blockSize)
	{
		ret = reinterpret_cast<reservoirHeaderS *>(
			__kbog->allocate(nBytes, flags) );

		if (ret != __KNULL)
		{
			/* Copy the bog header down so we don't overwrite it
			 * with the reservoir header.
			 **/
			memoryBogC::moveHeaderDown(
				ret, sizeof(reservoirHeaderS));

			ret->magic = RESERVOIR_MAGIC | RESERVOIR_FLAGS___KBOG;
			return reinterpret_cast<reservoirHeaderS *>(
				reinterpret_cast<uarch_t>( ret )
					+ sizeof(reservoirHeaderS) );
		};
	};

tryStream:
	// Unable to allocate from the kernel bog. Stream allocate.
	ret = new (processTrib.__kprocess.memoryStream.memAlloc(
		PAGING_BYTES_TO_PAGES(nBytes), 0)) reservoirHeaderS;

	if (ret != __KNULL)
	{
		ret->magic = RESERVOIR_MAGIC | RESERVOIR_FLAGS_STREAM;
		return reinterpret_cast<reservoirHeaderS *>(
			reinterpret_cast<uarch_t>( ret )
				+ sizeof(reservoirHeaderS) );
	};

	// If we're still here, then it means allocation completely failed.
	return __KNULL;
}

void memReservoirC::free(void *_mem)
{
	reservoirHeaderS	*mem;

	if (_mem == __KNULL) {
		return;
	};

	mem = reinterpret_cast<reservoirHeaderS *>(
		reinterpret_cast<uarch_t>( _mem ) - sizeof(reservoirHeaderS) );

	if ((mem->magic >> 4) != (RESERVOIR_MAGIC >> 4))
	{
		__kprintf(ERROR RESERVOIR"Corrupt memory or bad free v 0xS%X.\n",
			mem);

		return;
	};

	if (__KFLAG_TEST(
		(mem->magic & RESERVOIR_FLAGS_MASK), RESERVOIR_FLAGS_STREAM))
	{
		processTrib.__kprocess.memoryStream.memFree(mem);
		return;
	};

	if (__KFLAG_TEST(
		(mem->magic & RESERVOIR_FLAGS_MASK), RESERVOIR_FLAGS___KBOG))
	{
		if (__kbog == __KNULL) {
			return;
		};

		memoryBogC::moveHeaderUp(mem, sizeof(reservoirHeaderS));
		__kbog->free(mem);
		return;
	};

	__kprintf(WARNING RESERVOIR"free(0x%X): Operation fell through without "
		"finding subsystem to be freed to.\n", mem);
}

