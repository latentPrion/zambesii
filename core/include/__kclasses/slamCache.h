#ifndef _SLAM_CACHE_H
	#define _SLAM_CACHE_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/heapCache.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

/**	NOTES:
 * This idea, the Slam allocator, is designed by James Molloy for the Pedigree
 * (http://pedigree-project.org) project. Please make attempts to support
 * Pedigree.
 *
 *	EXPLANATION:
 * The heap algo works by keeping a free list and a partial list. The partial
 * list keeps a list of objects which have been free()d. The free list keeps a
 * list of blocks to be broken down (which have been release by an idle thread)
 * and can be quickly allocated from.
 *
 * One good thing about the Slam algorithm is that its very nature is such that
 * it caches to the extent that memory is needed. That is, when the heap is
 * detangled by an idle thread, the derived free blocks are placed on a list
 * local to that cache.
 *
 * The cache then re-uses that block list on the free list indefinitely.
 **/

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

