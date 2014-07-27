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
class PtrList
{
public:
	PtrList(sarch_t useCache=1);
	error_t initialize(void);
	~PtrList(void);

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
	struct sPtrListNode
	{
		T		*item;
		sPtrListNode	*next;
		ubit32		magic;
	};
	struct sPtrListState
	{
		sPtrListNode	*ptr;
		ubit32		nItems;
	};

	SharedResourceGroup<WaitLock, sPtrListState>	head;
	SlamCache		*cache;
	sarch_t			usingCache;
};


/**	Template Definition
 ******************************************************************************/

template <class T>
PtrList<T>::PtrList(sarch_t useCache)
:
cache(NULL), usingCache(useCache)
{
	head.rsrc.nItems = 0;
	head.rsrc.ptr = NULL;
}

template <class T>
error_t PtrList<T>::initialize(void)
{
	if (usingCache)
	{
		cache = cachePool.createCache(sizeof(sPtrListNode));
		if (cache == NULL) {
			return ERROR_MEMORY_NOMEM;
		};
	};

	return ERROR_SUCCESS;
}

template <class T>
PtrList<T>::~PtrList(void)
{
	sPtrListNode		*cur, *tmp;

	for (cur = head.rsrc.ptr; cur != NULL; )
	{
		tmp = cur;
		cur = cur->next;
		tmp->magic = 0;
		(usingCache) ? cache->free(tmp) : delete tmp;
	};
}

template <class T>
void PtrList<T>::dump(void)
{
	sPtrListNode	*tmp;

	head.lock.acquire();
	tmp = head.rsrc.ptr;
	printf(NOTICE PTRLIST"List obj @0x%p, usingCache? %d, cache @0x%p; "
		"%d items, 1st item @0x%p: "
		"Dumping.\n",
		this, usingCache, cache, head.rsrc.nItems, head.rsrc.ptr);

	for (; tmp != NULL; tmp = tmp->next)
	{
		printf(NOTICE PTRLIST"Node @0x%p, item: 0x%p, next: 0x%p.\n",
			tmp, tmp->item, tmp->next);
	};

	head.lock.release();
}


template <class T>
sarch_t PtrList<T>::checkForItem(T *item)
{
	sPtrListNode		*tmp;

	head.lock.acquire();

	for (tmp = head.rsrc.ptr; tmp != NULL; tmp = tmp->next)
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
ubit32 PtrList<T>::getNItems(void)
{
	ubit32		ret;

	head.lock.acquire();
	ret = head.rsrc.nItems;
	head.lock.release();

	return ret;
}

template <class T>
error_t PtrList<T>::insert(T *item)
{
	sPtrListNode		*node;

	node = (usingCache)
		? new (cache->allocate()) sPtrListNode
		: new sPtrListNode;

	if (node == NULL) {
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
sarch_t PtrList<T>::remove(T *item)
{
	sPtrListNode		*cur, *prev=NULL, *tmp;

	head.lock.acquire();

	cur = head.rsrc.ptr;
	for (; cur != NULL; )
	{
		if (cur->item == item)
		{
			tmp = cur;
			if (prev != NULL) {
				prev->next = cur->next;
			}
			else {
				head.rsrc.ptr = cur->next;
			};
			head.rsrc.nItems--;

			head.lock.release();

			tmp->magic = 0;
			(usingCache) ? cache->free(tmp) : delete tmp;
			return 1;
		};
		prev = cur;
		cur = cur->next;
	};

	head.lock.release();
	return 0;
}

template <class T>
void PtrList<T>::lock(void)
{
	head.lock.acquire();
}

template <class T>
void PtrList<T>::unlock(void)
{
	head.lock.release();
}

template <class T>
T *PtrList<T>::getItem(ubit32 num)
{
	sPtrListNode	*tmp;

	head.lock.acquire();
	// Cycle through until the counter is 0, or the list ends.
	for (tmp = head.rsrc.ptr;
		tmp != NULL && num > 0;
		tmp = tmp->next, num--)
	{};
	
	head.lock.release();

	// If the list ended before the counter ran out, return an error value.
	return (num > 0) ? NULL : tmp->item;
}

template <class T>
T *PtrList<T>::getNextItem(void **handle, ubit32 flags)
{
	sPtrListNode	*tmp = reinterpret_cast<sPtrListNode *>( *handle );
	T		*ret=NULL;

	if (handle == NULL) { return NULL; };

	// Don't allow arbitrary kernel memory reads.
	if (*handle != NULL
		&& ((sPtrListNode *)(*handle))->magic != PTRLIST_MAGIC) {
		return NULL;
	};

	if (!FLAG_TEST(flags, PTRLIST_FLAGS_NO_AUTOLOCK)) {
		head.lock.acquire();
	};

	// If starting a new walk:
	if (*handle == NULL)
	{
		*handle = head.rsrc.ptr;
		if (head.rsrc.ptr != NULL) { ret = head.rsrc.ptr->item; };
		if (!FLAG_TEST(flags, PTRLIST_FLAGS_NO_AUTOLOCK)) {
			head.lock.release();
		};

		return ret;
	};

	/**	FIXME:
	 * Optimally, each sPtrListNode object should have a magic number to
	 * distinguish between valid and discarded objects; this API allows
	 * for the caller to take and use items inside of it without locking
	 * the list off for the duration of their use.
	 *
	 * Another caller can possibly remove the item and free the memory,
	 * and then this next portion would be accessing illegal memory.
	 **/
	if (tmp->next != NULL)
	{
		*handle = tmp->next;

		if (!FLAG_TEST(flags, PTRLIST_FLAGS_NO_AUTOLOCK)) {
			head.lock.release();
		};

		return tmp->next->item;
	};

	if (!FLAG_TEST(flags, PTRLIST_FLAGS_NO_AUTOLOCK)) {
		head.lock.release();
	};

	return NULL;
}

#endif

