#ifndef _CACHE_POOL_H
	#define _CACHE_POOL_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/slamCache.h>
	#include <__kclasses/allocClass.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

/**	EXPLANATION:
 * A centralized repository of SLAM caches to be used to avoid duplication of
 * caches in the kernel. This is especially useful for the UDI subsystem which
 * will require allocation of memory blocks of all kinds of sizes, generally
 * smaller than a page.
 *
 * We won't be queueing UDI control blocks separately; Rather, we will have the
 * UDI subsystem allocate and free control blocks as usual, and the SLAM caches
 * will do the caching of objects with their implicit caching model.
 *
 * Any control blocks larger than a sensible fraction of a page should be
 * allocated on the heap, though.
 *
 *	CAVEAT:
 * AVOID creating caches whose object sizes are not multiples of sizeof(uarch_t)
 * for your kernel build. This will help you avoid lots of headaches.
 **/

#define CACHEPOOL				"Cache Pool: "

#define CACHEPOOL_STATUS_ALREADY_EXISTS		1
#define CACHEPOOL_SIZE_ROUNDUP(s)				\
	(((s) & (sizeof(uarch_t) - 1)) ? \
		(((s) + sizeof(uarch_t)) & (~(sizeof(uarch_t) - 1))) \
		: (s))

class cachePoolC
:
public AllocatorBase
{
private:
	struct cachePoolNodeS;

public:
	cachePoolC(void);
	error_t initialize(void) { return poolNodeCache.initialize(); };
	~cachePoolC(void);

	void dump(void);

public:
	SlamCache *getCache(uarch_t objSize);
	SlamCache *createCache(uarch_t objSize);
	void destroyCache(SlamCache *cache);

private:
	status_t insert(cachePoolNodeS *node);
	void remove(uarch_t objSize);

private:
	struct cachePoolNodeS
	{
		cachePoolNodeS	*next;
		SlamCache	*item;
	};

	sharedResourceGroupC<waitLockC, cachePoolNodeS *>	head;
	uarch_t		nCaches;
	SlamCache	poolNodeCache;
};

extern cachePoolC	cachePool;

#endif

