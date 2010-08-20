#ifndef _POOL_ALLOCATOR_H
	#define _POOL_ALLOCATOR_H

	#include <arch/arch.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/__kequalizerList.h>
	#include <__kclasses/slamCache.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/multipleReaderLock.h>

#ifdef __32_BIT__
	// "ZBZ ALLOC"
	#define ALLOCHEADER_MAGIC	0x2b2A110C
#elif defined(__64_BIT__)
	// "ZBZ ALLOC IS SO COOL"
	#define ALLOCHEADER_MAGIC	0x2b2A110C1550C001
#endif

#define ALLOCHEADER_SIZE		(sizeof(poolAllocatorC::allocHeaderS))
#define POOLALLOC			"Pool Allocator: "

class poolAllocatorC
{
public:
	poolAllocatorC(void);
	error_t initialize(void);

public:
	void *allocate(uarch_t size);
	void free(void *mem);

	void dump(void);

private:
	struct allocHeaderS
	{
		uarch_t		size;
		uarch_t		magic;
	};

	__kequalizerListC<slamCacheC>	caches;
	sharedResourceGroupC<multipleReaderLockC, uarch_t>	nCaches;
};

extern poolAllocatorC		poolAllocator;

#endif

