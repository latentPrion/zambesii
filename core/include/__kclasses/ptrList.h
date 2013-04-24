#ifndef _POINTER_LINKED_LIST_H
	#define _POINTER_LINKED_LIST_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kflagManipulation.h>
	#include <__kstdlib/__kcxxlib/new>
	#include <__kclasses/cachePool.h>
	#include <__kclasses/debugPipe.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

#define PTRLIST				"Pointer List: "

#define PTRLIST_MAGIC			0x1D1E1D11

#define PTRLIST_FLAGS_NO_AUTOLOCK	(1<<0)

template <class T>
class ptrListC
{
public:
	ptrListC(void);
	error_t initialize(void);
	~ptrListC(void);

public:
	error_t insert(T *item);
	sarch_t remove(T *item);

	ubit32 getNItems(void);

	sarch_t checkForItem(T *item);
	T *getNextItem(void **const handle, ubit32 flags=0);
	T *getItem(ubit32 num);

	void lock(void);
	void unlock(void);

	void dump(void);

private:
	/*void *operator new(size_t size)
	{
		if (cache != __KNULL) {
			return cache->allocate(size);
		};
	};

	void operator delete(void *mem)
	{
		if (cache != __KNULL) {
			cache->free(mem);
		};
	};*/

	struct ptrListNodeS
	{
		T		*item;
		ptrListNodeS	*next;
		ubit32		magic;
	};
	struct ptrListStateS
	{
		ptrListNodeS	*ptr;
		ubit32		nItems;
	};

	sharedResourceGroupC<waitLockC, ptrListStateS>	head;
	slamCacheC		*cache;
};


/**	Template Definition
 ******************************************************************************/

template <class T>
ptrListC<T>::ptrListC(void)
{
	head.rsrc.nItems = 0;
	head.rsrc.ptr = __KNULL;
	cache = __KNULL;
}

template <class T>
error_t ptrListC<T>::initialize(void)
{
	cache = cachePool.createCache(sizeof(ptrListNodeS));
	if (cache == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};

	return ERROR_SUCCESS;
}

template <class T>
ptrListC<T>::~ptrListC(void)
{
	ptrListNodeS		*cur, *tmp;

	for (cur = head.rsrc.ptr; cur != __KNULL; )
	{
		tmp = cur;
		cur = cur->next;
		tmp->magic = 0;
		cache->free(tmp);
	};
}

template <class T>
void ptrListC<T>::dump(void)
{
	ptrListNodeS	*tmp;

	head.lock.acquire();
	tmp = head.rsrc.ptr;
	__kprintf(NOTICE PTRLIST"List obj @0x%p, %d items, 1st item @0x%p: "
		"Dumping.\n",
		this, head.rsrc.nItems, head.rsrc.ptr);

	for (; tmp != __KNULL; tmp = tmp->next)
	{
		__kprintf(NOTICE PTRLIST"Node @0x%p, item: 0x%p, next: 0x%p.\n",
			tmp, tmp->item, tmp->next);
	};

	head.lock.release();
}


template <class T>
sarch_t ptrListC<T>::checkForItem(T *item)
{
	ptrListNodeS		*tmp;

	head.lock.acquire();

	for (tmp = head.rsrc.ptr; tmp != __KNULL; tmp = tmp->next)
	{
		if (tmp->item == item) {
			head.lock.release();
			return 1;
		};
	};

	head.lock.release();
	return 0;		
}

template <class T>
ubit32 ptrListC<T>::getNItems(void)
{
	ubit32		ret;

	head.lock.acquire();
	ret = head.rsrc.nItems;
	head.lock.release();

	return ret;
}

template <class T>
error_t ptrListC<T>::insert(T *item)
{
	ptrListNodeS		*node;

	node = new (cache->allocate()) ptrListNodeS;
	if (node == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};
	node->magic = PTRLIST_MAGIC;
	node->item = item;

	head.lock.acquire();

	node->next = head.rsrc.ptr;
	head.rsrc.ptr = node;

	head.rsrc.nItems++;

	head.lock.release();
	return ERROR_SUCCESS;
}

template <class T>
sarch_t ptrListC<T>::remove(T *item)
{
	ptrListNodeS		*cur, *prev=__KNULL, *tmp;

	head.lock.acquire();

	cur = head.rsrc.ptr;
	for (; cur != __KNULL; )
	{
		if (cur->item == item)
		{
			tmp = cur;
			if (prev != __KNULL) {
				prev->next = cur->next;
			}
			else {
				head.rsrc.ptr = cur->next;
			};
			head.rsrc.nItems--;

			head.lock.release();
			tmp->magic = 0;
			cache->free(tmp);
			return 1;
		};
		prev = cur;
		cur = cur->next;
	};

	head.lock.release();
	return 0;
}

template <class T>
void ptrListC<T>::lock(void)
{
	head.lock.acquire();
}

template <class T>
void ptrListC<T>::unlock(void)
{
	head.lock.release();
}

template <class T>
T *ptrListC<T>::getItem(ubit32 num)
{
	ptrListNodeS	*tmp;

	head.lock.acquire();
	// Cycle through until the counter is 0, or the list ends.
	for (tmp = head.rsrc.ptr;
		tmp != __KNULL && num > 0;
		tmp = tmp->next, num--)
	{};
	
	head.lock.release();

	// If the list ended before the counter ran out, return an error value.
	return (num > 0) ? __KNULL : tmp->item;
}

template <class T>
T *ptrListC<T>::getNextItem(void **handle, ubit32 flags)
{
	ptrListNodeS	*tmp = reinterpret_cast<ptrListNodeS *>( *handle );
	T		*ret=__KNULL;

	// Don't allow arbitrary kernel memory reads.
	if (*handle != __KNULL
		&& ((ptrListNodeS *)(*handle))->magic != PTRLIST_MAGIC) {
		return __KNULL;
	};

	if (!__KFLAG_TEST(flags, PTRLIST_FLAGS_NO_AUTOLOCK)) {
		head.lock.acquire();
	};

	// If starting a new walk:
	if (*handle == __KNULL)
	{
		*handle = head.rsrc.ptr;
		if (head.rsrc.ptr != __KNULL) { ret = head.rsrc.ptr->item; };
		if (!__KFLAG_TEST(flags, PTRLIST_FLAGS_NO_AUTOLOCK)) {
			head.lock.release();
		};

		return ret;
	};

	/**	FIXME:
	 * Optimally, each ptrListNodeS object should have a magic number to
	 * distinguish between valid and discarded objects; this API allows
	 * for the caller to take and use items inside of it without locking
	 * the list off for the duration of their use.
	 *
	 * Another caller can possibly remove the item and free the memory,
	 * and then this next portion would be accessing illegal memory.
	 **/
	if (tmp->next != __KNULL)
	{
		*handle = tmp->next;

		if (!__KFLAG_TEST(flags, PTRLIST_FLAGS_NO_AUTOLOCK)) {
			head.lock.release();
		};

		return tmp->next->item;
	};

	if (!__KFLAG_TEST(flags, PTRLIST_FLAGS_NO_AUTOLOCK)) {
		head.lock.release();
	};

	return __KNULL;
}

#endif

