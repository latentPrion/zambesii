#ifndef _ZAMBEZII_MEMORY_RESERVOIR_H
	#define _ZAMBEZII_MEMORY_RESERVOIR_H

	#include <arch/arch.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/__kequalizerList.h>
	#include <__kclasses/slamCache.h>
	#include <__kclasses/heap.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/multipleReaderLock.h>

// For any word size, the LAST 4 BITS are RESESRVED.
#ifdef __32_BIT__
	// "ZBZ ALLO[C]"
	#define RESERVOIR_MAGIC	0x2b2A1100
#elif defined(__64_BIT__)
	// "ZBZ ALLOC IS SO COO[L]"
	#define RESERVOIR_MAGIC	0x2b2A110C1550C000
#endif

#define RESERVOIR_FLAGS___KBOG		(1<<0)
#define RESERVOIR_FLAGS_STREAM		(1<<1)
#define RESERVOIR_FLAGS_MASK		(0xF)

#define RESERVOIR_MAX_NBOGS		(PAGING_BASE_SIZE / sizeof(void *))
#define RESERVOIR			"Mem Reservoir: "

class memoryStreamC;

class memReservoirC
{
public:
	memReservoirC(memoryStreamC *sourceStream);
	error_t initialize(void);
	~memReservoirC(void);

public:
	// Allocates from the default bog.
	void *allocate(uarch_t nBytes, uarch_t flags=0);
	// Allocates from the bog with the handle passed.
	void *allocate(void *bogHandle, uarch_t nBytes, uarch_t flags=0);
	void *reallocate(void *old, uarch_t nBytes, uarch_t flags=0);
	void free(void *mem);
	void free(void *bogHandle, void *mem);

	error_t checkBogAllocations(sarch_t nBytes);
	error_t checkHeapAllocations(void)
	{
		return __kheap.checkAllocations();
	}

	heapC *__kgetHeap(void) { return &__kheap; }
	void dump(void);

private:
	struct bogStateS
	{
		heapC		**ptrs;
		uarch_t		nHeaps;
	};
	struct reservoirHeaderS
	{
		// Modifying this has consequences in the heap.
		uarch_t		magic;
	};

	heapC			__kheap;
	memoryStreamC		*sourceStream;
	sharedResourceGroupC<multipleReaderLockC, bogStateS>	heaps;
};

extern memReservoirC		memReservoir;

#endif

