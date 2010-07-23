#ifndef _SLAM_CACHE_H
	#define _SLAM_CACHE_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/heapCache.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

class slamCacheC
:
public heapCacheC
{
public:
	slamCacheC(void);
	slamCacheC(uarch_t objectSize);
	error_t initialize(uarch_t objectSize);
	virtual ~slamCacheC(void);

public:
	virtual void *allocate(void);
	virtual void free(void *ptr);

private:
	ubit32		perPageExcess;
	ubit32		perPageBlocks;
	struct object
	{
#ifdef CONFIG_HEAP_SLAM_DEBUG
		uarch_t		magic;
#endif
		object		*next;
	};

	sharedResourceGroupC<waitLockC, object *>	partialList, freeList;
};

#endif

