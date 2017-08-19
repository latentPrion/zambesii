
#include <__kclasses/cachePool.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>


CachePool::CachePool(void)
:
nCaches(0), poolNodeCache(sizeof(sCachePoolNode))
{
	head.rsrc = NULL;
}

CachePool::~CachePool(void)
{
	sCachePoolNode		*cur, *tmp;

	head.lock.acquire();

	for (cur = head.rsrc; cur != NULL; )
	{
		delete cur->item;
		tmp = cur;
		cur = cur->next;
		poolNodeCache.free(tmp);
	};
}

void CachePool::dump(void)
{
	printf(NOTICE CACHEPOOL"Dumping. nCaches: %d. Dump nodecache:\n",
		nCaches);

	poolNodeCache.dump();

	head.lock.acquire();

	for (sCachePoolNode *cur = head.rsrc; cur != NULL; cur = cur->next)
	{
		printf(NOTICE CACHEPOOL"Node: %p, Item %p, size %p.\n",
			cur, cur->item, cur->item->sObjectize);
	};

	head.lock.release();
}

status_t CachePool::insert(sCachePoolNode *node)
{
	sCachePoolNode		*cur, *prev;

	prev = NULL;

	head.lock.acquire();

	// No items in list.
	if (head.rsrc == NULL)
	{
		head.rsrc = node;
		node->next = NULL;
		nCaches++;

		head.lock.release();
		return ERROR_SUCCESS;
	};

	cur = head.rsrc;
	for (; cur != NULL; )
	{
		if (node->item->sObjectize < cur->item->sObjectize)
		{
			// If adding before first item in list.
			if (prev != NULL) {
				prev->next = node;
			}
			// Else adding in middle.
			else {
				head.rsrc = node;
			};
			node->next = cur;
			nCaches++;

			head.lock.release();
			return ERROR_SUCCESS;
		};

		if (cur->item->sObjectize == node->item->sObjectize)
		{
			// Cache already exists. Release lock and exit.
			head.lock.release();
			return CACHEPOOL_STATUS_ALREADY_EXISTS;
		};

		prev = cur;
		cur = cur->next;
	};

	// Adding at end of list.
	if (prev != NULL)
	{
		prev->next = node;
		node->next = NULL;
		nCaches++;

		head.lock.release();
		return ERROR_SUCCESS;
	};

	// If we reach here, something really weird has happened.
	head.lock.release();
	return ERROR_UNKNOWN;
}

void CachePool::remove(uarch_t objSize)
{
	sCachePoolNode		*cur, *prev;

	objSize = CACHEPOOL_SIZE_ROUNDUP(objSize);
	prev = NULL;

	head.lock.acquire();

	cur = head.rsrc;
	for (; cur != NULL; )
	{
		if (cur->item->sObjectize > objSize)
		{
			// Cache size doesn't exist.
			head.lock.release();
			return;
		};

		if (cur->item->sObjectize == objSize)
		{
			// If removing first item in list.
			if (prev == NULL)
			{
				head.rsrc = cur->next;
				nCaches--;

				head.lock.release();
				delete cur->item;
				poolNodeCache.free(cur);
				return;
			}
			// Else removing from middle.
			else
			{
				prev->next = cur->next;
				nCaches--;

				head.lock.release();
				delete cur->item;
				poolNodeCache.free(cur);
				return;
			};
		};

		prev = cur;
		cur = cur->next;
	};

	// No items in list.
	head.lock.release();
}

SlamCache *CachePool::getCache(uarch_t objSize)
{
	sCachePoolNode		*cur;

	objSize = CACHEPOOL_SIZE_ROUNDUP(objSize);

	head.lock.acquire();

	cur = head.rsrc;
	for (; cur != NULL; cur = cur->next)
	{
		if (cur->item->sObjectize == objSize)
		{
			head.lock.release();
			return cur->item;
		};
	};

	head.lock.release();
	return NULL;
}

SlamCache *CachePool::createCache(uarch_t objSize)
{
	sCachePoolNode	*node;
	status_t	status;
	SlamCache	*ret;

	objSize = CACHEPOOL_SIZE_ROUNDUP(objSize);

	node = new (poolNodeCache.allocate()) sCachePoolNode;
	if (node == NULL) {
		return NULL;
	};

	node->item = new SlamCache(objSize);

	if (node->item == NULL)
	{
		ret = NULL;
		goto releaseNode;
	};

	// Now add it to the list.
	status = insert(node);
	if (status < 0)
	{
		ret = NULL;
		goto releaseCache;
	};

	return getCache(objSize);

releaseCache:
	delete node->item;

releaseNode:
	poolNodeCache.free(node);
	return ret;
}

void CachePool::destroyCache(SlamCache *cache)
{
	remove(CACHEPOOL_SIZE_ROUNDUP(cache->sObjectize));
}

