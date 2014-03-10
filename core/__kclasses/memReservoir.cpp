
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


memReservoirC::memReservoirC(memoryStreamC *sourceStream)
:
__kheap(
	CHIPSET_MEMORY___KBOG_SIZE, sourceStream,
	0
	| heapC::OPT_GUARD_PAGED
	| heapC::OPT_CHECK_ALLOCS_ON_FREE
	| heapC::OPT_CHECK_BLOCK_MAGIC_PASSIVELY
	| heapC::OPT_CHECK_HEAP_RANGES_ON_FREE),
sourceStream(sourceStream)
{
	heaps.rsrc.ptrs = NULL;
	heaps.rsrc.nHeaps = 0;
}

error_t memReservoirC::initialize(void)
{
	error_t		ret;

	ret = __kheap.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	heaps.rsrc.ptrs = new (
		sourceStream->memAlloc(1, MEMALLOC_NO_FAKEMAP)) heapC*;

	if (heaps.rsrc.ptrs == NULL)
	{
		printf(ERROR RESERVOIR"Unable to allocate a page to hold "
			"the array of custom bog allocators.\n");

		return ERROR_MEMORY_NOMEM;
	};

	memset(heaps.rsrc.ptrs, 0, PAGING_BASE_SIZE);

	printf(NOTICE RESERVOIR"initialize(): Done. __kheap chunk size 0x%x, "
		"custom bogs array 0x%p.\n",
		__kheap.getChunkSize(), heaps.rsrc.ptrs);

	return ERROR_SUCCESS;
}

memReservoirC::~memReservoirC(void)
{
}

void memReservoirC::dump(void)
{

	printf(NOTICE RESERVOIR"Dumping.\n");
	printf(NOTICE RESERVOIR"bogs array 0x%X, nHeaps %d.\n",
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

error_t memReservoirC::checkBogAllocations(sarch_t)
{
	__kheap.dumpAllocations();
	return ERROR_SUCCESS;
}

void *memReservoirC::allocate(uarch_t nBytes, uarch_t)
{
	reservoirHeaderS	*ret;

	if (nBytes == 0)
	{
		printf(WARNING RESERVOIR"Caller 0x%p.\n",
			__builtin_return_address(1));

		panic(WARNING RESERVOIR"Allocation with size=0.\n");
	};

	nBytes += sizeof(reservoirHeaderS);

	if (nBytes <= __kheap.getChunkSize())
	{
		ret = new (__kheap.malloc(
			nBytes, __builtin_return_address(1))) reservoirHeaderS;

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
			PAGING_BYTES_TO_PAGES(nBytes), 0)) reservoirHeaderS;

	if (ret != NULL)
	{
		ret->magic = RESERVOIR_MAGIC | RESERVOIR_FLAGS_STREAM;
		return ret + 1;
	};

	// If we're still here, then it means allocation completely failed.
	return NULL;
}

void memReservoirC::free(void *_mem)
{
	reservoirHeaderS	*mem;

	if (_mem == NULL) {
		return;
	};

	mem = reinterpret_cast<reservoirHeaderS *>( _mem );
	mem--;

	if ((mem->magic >> 4) != (RESERVOIR_MAGIC >> 4))
	{
		printf(ERROR RESERVOIR"Corrupt memory or bad free v 0x%p.\n",
			mem);

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

	printf(WARNING RESERVOIR"free(0x%X): Operation fell through without "
		"finding subsystem to be freed to.\n", mem);
}

