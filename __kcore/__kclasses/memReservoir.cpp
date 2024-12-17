
#include <debug.h>
#include <arch/paging.h>
#include <chipset/memory.h>
#include <__kstdlib/__kmath.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <__kclasses/memReservoir.h>
#include <kernel/common/panic.h>
#include <kernel/common/processTrib/processTrib.h>


MemReservoir::MemReservoir(MemoryStream *sourceStream)
:
__kheap(
	CHIPSET_MEMORY___KBOG_SIZE, sourceStream,
	0
	| Heap::OPT_GUARD_PAGED
	| Heap::OPT_CHECK_ALLOCS_ON_FREE
	| Heap::OPT_CHECK_BLOCK_MAGIC_PASSIVELY
	| Heap::OPT_CHECK_HEAP_RANGES_ON_FREE),
sourceStream(sourceStream)
{
	heaps.rsrc.ptrs = NULL;
	heaps.rsrc.nHeaps = 0;
}

error_t MemReservoir::initialize(void)
{
	error_t		ret;

	ret = __kheap.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	heaps.rsrc.ptrs = new (
		sourceStream->memAlloc(1, MEMALLOC_NO_FAKEMAP)) Heap*;

	if (heaps.rsrc.ptrs == NULL)
	{
		printf(ERROR RESERVOIR"Unable to allocate a page to hold "
			"the array of custom bog allocators.\n");

		return ERROR_MEMORY_NOMEM;
	};

	memset(heaps.rsrc.ptrs, 0, PAGING_BASE_SIZE);

	printf(NOTICE RESERVOIR"initialize(): Done. __kheap chunk size %x, "
		"custom bogs array %p.\n",
		__kheap.getChunkSize(), heaps.rsrc.ptrs);

	zkcmCore.chipsetEventNotification(__KPOWER_EVENT_HEAP_AVAIL, 0);
	return ERROR_SUCCESS;
}

MemReservoir::~MemReservoir(void)
{
}

void MemReservoir::dump(void)
{

	printf(NOTICE RESERVOIR"Dumping.\n");
	printf(NOTICE RESERVOIR"bogs array %X, nHeaps %d.\n",
		heaps.rsrc.ptrs, heaps.rsrc.nHeaps);

	printf(NOTICE RESERVOIR"Dumping __kheap.\n");
	__kheap.dump();

	for (uarch_t i=0; i<heaps.rsrc.nHeaps; i++)
	{
		printf(NOTICE RESERVOIR"Dumping bog %d @ v %X.\n",
			i, heaps.rsrc.ptrs[i]);

		heaps.rsrc.ptrs[i]->dump();
	};
}

error_t MemReservoir::checkBogAllocations(sarch_t)
{
	__kheap.dumpAllocations();
	return ERROR_SUCCESS;
}

void *MemReservoir::allocate(uarch_t nBytes, uarch_t)
{
	sReservoirHeader	*ret;

	if (nBytes == 0)
	{
		printf(WARNING RESERVOIR"Caller %p.\n",
			__builtin_return_address(1));

		panic(WARNING RESERVOIR"Allocation with size=0.\n");
	};

	nBytes += sizeof(sReservoirHeader);

	if (nBytes <= __kheap.getChunkSize())
	{
		ret = new (__kheap.malloc(
			nBytes, __builtin_return_address(1))) sReservoirHeader;

		if (ret != NULL)
		{
			ret->magic =
				RESERVOIR_MAGIC | RESERVOIR_FLAGS___KBOG;

			ret++;
			return ret;
		};
	};

	// Unable to allocate from the kernel bog. Stream allocate.
	ret = new (
		sourceStream->memAlloc(
			PAGING_BYTES_TO_PAGES(nBytes), 0)) sReservoirHeader;

	if (ret != NULL)
	{
		ret->magic = RESERVOIR_MAGIC | RESERVOIR_FLAGS_STREAM;
		return ret + 1;
	};

	// If we're still here, then it means allocation completely failed.
	return NULL;
}

void *MemReservoir::reallocate(void *old, uarch_t _nBytes, uarch_t)
{
	sReservoirHeader	*resHdr = (sReservoirHeader *)old;
	Heap::Allocation	*heapHdr;
	uarch_t			nBytes = _nBytes + sizeof(sReservoirHeader);

	(void)heapHdr;

	/**	EXPLANATION:
	 * Problem here is that we need to have the MemReservoir header on the
	 * allocation, even for allocations returned by reallocate().
	 *
	 * I was not in the mood when I wrote this code, so I simply decided
	 * to write a suboptimal implementation where we add the header size to
	 * the requested allocation size.
	 **/
	resHdr--;
	heapHdr = (Heap::Allocation *)resHdr;
	heapHdr--;

	if ((resHdr->magic >> 4) != (RESERVOIR_MAGIC >> 4))
	{
		printf(ERROR RESERVOIR"realloc: Corrupt memory or bad free v %p.\n",
			old);

		panic(ERROR_FATAL);
	};

	return __kheap.realloc(old, nBytes);
}

void MemReservoir::free(void *_mem)
{
	sReservoirHeader	*mem;

	if (_mem == NULL) {
		return;
	};

	mem = reinterpret_cast<sReservoirHeader *>( _mem );
	mem--;

	if ((mem->magic >> 4) != (RESERVOIR_MAGIC >> 4))
	{
		printf(ERROR RESERVOIR"free: Corrupt memory or bad free v%p. "
			"Caller: %p.\n",
			mem, __builtin_return_address(1));

		return;
	};

	if (FLAG_TEST(
		(mem->magic & RESERVOIR_FLAGS_MASK), RESERVOIR_FLAGS_STREAM))
	{
		sourceStream->memFree(mem);
		return;
	};

	if (FLAG_TEST(
		(mem->magic & RESERVOIR_FLAGS_MASK), RESERVOIR_FLAGS___KBOG))
	{
		__kheap.free(mem, __builtin_return_address(1));
		return;
	};

	printf(WARNING RESERVOIR"free(%X): Operation fell through without "
		"finding subsystem to be freed to.\n", mem);
}

