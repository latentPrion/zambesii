#ifndef _SLAM_CACHE_H
	#define _SLAM_CACHE_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/heapCache.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/recursiveLock.h>
	#include <kernel/common/waitLock.h>

/**	NOTES:
 * This idea, the Slam allocator, is designed by James Molloy for the Pedigree
 * (http://pedigree-project.org) project. Please make attempts to support
 * Pedigree and follow it, as it's a worthwhile project by any standard.
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

#define SLAMCACHE		"Slam Cache: "

#define SLAMCACHE_MAGIC				0x51A111CA
#define SLAMCACHE_ALLOC_LOCAL_FLUSH_ONLY	(1<<0)

class memReservoirC;

class slamCacheC
:
public heapCacheC
{
friend class memReservoirC;
public:
	/* Used to tell the cache whether it should allocate new pages directly
	 * from the Memory Tributary (rawMemAlloc()/rawMemFree()) or from the
	 * kernel's Memory Stream (memAlloc()/memFree()).
	 *
	 * The default is the kernel Memory Stream. If you don't know if or why
	 * you need to allocate using rawMemAlloc(), you most likely don't need
	 * to.
	 **/
	enum allocatorE { RAW, STREAM };
	slamCacheC(uarch_t objectSize, allocatorE allocator=STREAM);

	error_t initialize(void) { return ERROR_SUCCESS; }
	virtual ~slamCacheC(void);

public:
	virtual void *allocate(uarch_t flags=0, ubit8 *requiredNewPage=NULL);
	virtual void free(void *ptr);

	status_t detangle(void);
	status_t flush(void);

	void dump(void);

private:
	struct objectS
	{
		objectS(void)
#ifdef CONFIG_HEAP_SLAM_DEBUG
		:
		magic(SLAMCACHE_MAGIC),
#endif
		{}

		~objectS(void)
		{
#ifdef CONFIG_HEAP_SLAM_DEBUG
			magic = 0;
#endif
		}

#ifdef CONFIG_HEAP_SLAM_DEBUG
		uarch_t		magic;
#endif
		objectS		*next;
	};

	void *getNewPage(sarch_t localFlush);
	void releasePage(void *page);

private:
	ubit32		perPageExcess;
	ubit32		perPageBlocks;
	allocatorE	allocator;

	sharedResourceGroupC<waitLockC, objectS *>	partialList, freeList;
};

#endif

