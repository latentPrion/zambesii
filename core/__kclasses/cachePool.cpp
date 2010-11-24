
#include <__kclasses/cachePool.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>


cachePoolC::cachePoolC(void)
:
poolNodeCache(sizeof(cachePoolNodeS))
{
	head.rsrc = __KNULL;
	nCaches = 0;
}

cachePoolC::~cachePoolC(void)
{
	cachePoolNodeS		*cur, *tmp;

	head.lock.acquire();

	for (cur = head.rsrc; cur != __KNULL; )
	{
		delete cur->item;
		tmp = cur;
		cur = cur->next;
		poolNodeCache.free(tmp);
	};
}

void cachePoolC::dump(void)
{
	__kprintf(NOTICE CACHEPOOL"Dumping.\t\tNCaches: %d.\n", nCaches);

	head.lock.acquire();

	for (cachePoolNodeS *cur = head.rsrc; cur != __KNULL; cur = cur->next)
	{
		__kprintf(NOTICE CACHEPOOL"Node: 0x%p, Item 0x%p, size 0x%p.\n",
			cur, cur->item, cur->item->objectSize);
	};

	head.lock.release();
}

status_t cachePoolC::insert(cachePoolNodeS *node)
{
	cachePoolNodeS		*cur, *prev;

	prev = __KNULL;

	head.lock.acquire();

	// No items in list.
	if (head.rsrc == __KNULL)
	{
		head.rsrc = node;
		node->next = __KNULL;
		nCaches++;

		head.lock.release();
		return ERROR_SUCCESS;
	};

	cur = head.rsrc;
	for (; cur != __KNULL; )
	{
		if (node->item->objectSize < cur->item->objectSize)
		{
			// If adding before first item in list.
			if (prev != __KNULL) {
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

		if (cur->item->objectSize == node->item->objectSize)
		{
			// Cache already exists. Release lock and exit.
			head.lock.release();
			return CACHEPOOL_STATUS_ALREADY_EXISTS;
		};

		prev = cur;
		cur = cur->next;
	};

	// Adding at end of list.
	if (prev != __KNULL)
	{
		prev->next = node;
		node->next = __KNULL;
		nCaches++;

		head.lock.release();
		return ERROR_SUCCESS;
	};

	// If we reach here, something really weird has happened.
	head.lock.release();
	return ERROR_UNKNOWN;
}

void cachePoolC::remove(uarch_t objSize)
{
	cachePoolNodeS		*cur, *prev;

	objSize = CACHEPOOL_SIZE_ROUNDUP(objSize);
	prev = __KNULL;

	head.lock.acquire();

	cur = head.rsrc;
	for (; cur != __KNULL; )
	{
		if (cur->item->objectSize > objSize)
		{
			// Cache size doesn't exist.
			head.lock.release();
			return;
		};

		if (cur->item->objectSize == objSize)
		{
			// If removing first item in list.
			if (prev == __KNULL)
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

slamCacheC *cachePoolC::getCache(uarch_t objSize)
{
	cachePoolNodeS		*cur;

	objSize = CACHEPOOL_SIZE_ROUNDUP(objSize);

	head.lock.acquire();

	cur = head.rsrc;
	for (; cur != __KNULL; cur = cur->next)
	{
		if (cur->item->objectSize == objSize)
		{
			head.lock.release();
			return cur->item;
		};
	};

	head.lock.release();
	return __KNULL;
}

slamCacheC *cachePoolC::createCache(uarch_t objSize)
{
	cachePoolNodeS	*node;
	status_t	status;

	objSize = CACHEPOOL_SIZE_ROUNDUP(objSize);

	node = new (poolNodeCache.allocate()) cachePoolNodeS;
	if (node == __KNULL) {
		return __KNULL;
	};

	node->item = new slamCacheC(objSize);
	if (node->item == __KNULL) {
		goto releaseNode;
	};

	// Now add it to the list.
	status = insert(node);
	if (status < 0)
	{
		ret = __KNULL;
		goto releaseCache;
	};

	ret = getCache(objSize);

releaseCache:
	delete node->item;

releaseNode:
	poolNodeCache.free(node);
	return ret;
}

void cachePoolC::destroyCache(slamCacheC *cache)
{
	remove(CACHEPOOL_SIZE_ROUNDUP(cache->objectSize));
}

