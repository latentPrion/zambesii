#ifndef _ZAMBEZII_MEMORY_RESERVOIR_H
	#define _ZAMBEZII_MEMORY_RESERVOIR_H

	#include <arch/arch.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/__kequalizerList.h>
	#include <__kclasses/slamCache.h>
	#include <__kclasses/memoryBog.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/multipleReaderLock.h>

#ifdef __32_BIT__
	// "ZBZ ALLOC"
	#define RESERVOIR_HEADER_MAGIC	0x2b2A110C
#elif defined(__64_BIT__)
	// "ZBZ ALLOC IS SO COOL"
	#define RESERVOIR_HEADER_MAGIC	0x2b2A110C1550C001
#endif

#define RESERVOIR_MAX_NBOGS		(PAGING_BASE_SIZE / sizeof(void *))
#define RESERVOIR_MAX_NCACHES		(PAGING_BASE_SIZE / sizeof(void *))
#define RESERVOIR			"Mem Reservoir: "

class memReservoirC
{
public:
	memReservoirC(void);
	error_t initialize(void);
	~memReservoirC(void);

public:
	// Allocates from the default bog.
	void *allocate(uarch_t nBytes, uarch_t flags=0);
	// Allocates from the bog with the handle passed.
	void *allocate(void *bogHandle, uarch_t nBytes, uarch_t flags=0);
	void free(void *mem);
	void free(void *bogHandle, void *mem);

	void dump(void);

private:
	// Creates a new slam cache and adds it to the array of slam caches.
	slamCacheC *createCache(uarch_t objSize);

	struct allocHeaderS
	{
		uarch_t		size;
		uarch_t		magic;
	};

	struct cacheStateS
	{
		slamCacheC	**ptrs;
		uarch_t		nCaches;
	};
	struct bogStateS
	{
		memoryBogC	**ptrs;
		uarch_t		nBogs;
	};
	struct reservoirHeaderS
	{
		slamCacheC	*owner;
	};
	sharedResourceGroupC<multipleReaderLockC, cacheStateS>	caches;
	memoryBogC	*__kbog;
	sharedResourceGroupC<multipleReaderLockC, bogStateS>	bogs;
};

extern memReservoirC		memReservoir;

#endif

