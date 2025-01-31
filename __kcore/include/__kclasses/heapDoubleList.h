#ifndef _POINTER_DOUBLY_LINKED_LIST_H
	#define _POINTER_DOUBLY_LINKED_LIST_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kflagManipulation.h>
	#include <__kstdlib/__kcxxlib/new>
	#include <__kclasses/debugPipe.h>
	#include <__kclasses/cachePool.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

#define PTRDBLLIST			"Dbl-Link HeapList: "

#define PTRDBLLIST_INITIALIZE_FLAGS_USE_OBJECT_CACHE	(1<<0)

#define PTRDBLLIST_ADD_HEAD		0x0
#define PTRDBLLIST_ADD_TAIL		0x1

template <class T>
class HeapDoubleList
{
public:
	HeapDoubleList(void)
	:
	objectCache(NULL)
	{
		list.rsrc.head = list.rsrc.tail = NULL;
		list.rsrc.nItems = 0;
	}

	~HeapDoubleList(void)
	{
		cachePool.destroyCache(objectCache);
	};

	error_t initialize(ubit32 flags=0)
	{
		if (FLAG_TEST(flags,
			PTRDBLLIST_INITIALIZE_FLAGS_USE_OBJECT_CACHE))
		{
			objectCache = cachePool.createCache(sizeof(sListNode));
			if (objectCache == NULL) {
				return ERROR_MEMORY_NOMEM;
			};
		};

		return ERROR_SUCCESS;
	};

public:
	void dump(void);

	error_t addItem(T *item, ubit8 mode=PTRDBLLIST_ADD_TAIL);
	void removeItem(T *item);
	T *popFromHead(void);
	T *popFromTail(void);
	T *getHead(void);
	T *getTail(void);

	uarch_t	getNItems(void);

protected:
	struct sListNode
	{
		T		*item;
		sListNode	*prev, *next;
	};

	struct sListState
	{
		sListNode	*head, *tail;
		uarch_t		nItems;
	};

	SharedResourceGroup<WaitLock, sListState>	list;
	SlamCache		*objectCache;
};


/**	Template definition.
 ******************************************************************************/

template <class T>
uarch_t HeapDoubleList<T>::getNItems(void)
{
	uarch_t		ret;

	list.lock.acquire();
	ret = list.rsrc.nItems;
	list.lock.release();

	return ret;
}

template <class T>
void HeapDoubleList<T>::dump(void)
{
	sListNode	*curr, *head, *tail;
	sbit8		flipFlop=0;

	list.lock.acquire();
	head = list.rsrc.head;
	tail = list.rsrc.tail;
	list.lock.release();

	printf(NOTICE PTRDBLLIST"@ head %p, tail %p, %d items, "
		"lock obj %p: Dumping.\n",
		head, tail, getNItems(), &list.lock);

	list.lock.acquire();

	curr = head;
	for (; curr != NULL; curr = curr->next, flipFlop++)
	{
		if (flipFlop < 4) { printf(CC"\t%p ", curr->item); }
		else
		{
			printf(CC"%p\n", curr->item);
			flipFlop = -1;
		};
	};

	if (flipFlop != 0) { printf(CC"\n"); };
	list.lock.release();
}

template <class T>
error_t HeapDoubleList<T>::addItem(T *item, ubit8 mode)
{
	sListNode	*newNode;

	newNode = (objectCache == NULL)
		? new sListNode
		: new (objectCache->allocate()) sListNode;

	if (newNode == NULL)
	{
		printf(ERROR PTRDBLLIST"addItem(%p,%s): Failed to alloc "
			"mem for new node.\n",
			item, (mode == PTRDBLLIST_ADD_HEAD)?"head":"tail");

		return ERROR_MEMORY_NOMEM;
	};

	newNode->item = item;

	switch (mode)
	{
	case PTRDBLLIST_ADD_HEAD:
		newNode->prev = NULL;
		
		list.lock.acquire();

		newNode->next = list.rsrc.head;
		if (list.rsrc.head != NULL) {
			list.rsrc.head->prev = newNode;
		}
		else { list.rsrc.tail = newNode; };
		list.rsrc.head = newNode;
		list.rsrc.nItems++;

		list.lock.release();
		break;

	case PTRDBLLIST_ADD_TAIL:
		newNode->next = NULL;

		list.lock.acquire();

		newNode->prev = list.rsrc.tail;
		if (list.rsrc.tail != NULL) {
			list.rsrc.tail->next = newNode;
		}
		else { list.rsrc.head = newNode; };
		list.rsrc.tail = newNode;
		list.rsrc.nItems++;

		list.lock.release();
		break;

	default:
		printf(ERROR PTRDBLLIST"addItem(%p): Invalid add mode.\n",
			item);

		return ERROR_INVALID_ARG;
	};

	return ERROR_SUCCESS;
}

template <class T>
void HeapDoubleList<T>::removeItem(T *item)
{
	sListNode	*curr;

	list.lock.acquire();
	// Just run through until we find it, then remove it.
	for (curr = list.rsrc.head; curr != NULL; curr = curr->next)
	{
		if (curr->item == item)
		{
			if (list.rsrc.head == curr)
			{
				list.rsrc.head = curr->next;
				if (list.rsrc.head != NULL) {
					curr->next->prev = NULL;
				};
			}
			else { curr->prev->next = curr->next; };

			if (list.rsrc.tail == curr)
			{
				list.rsrc.tail = curr->prev;
				if (list.rsrc.tail != NULL) {
					curr->prev->next = NULL;
				};
			}
			else { curr->next->prev = curr->prev; };

			list.rsrc.nItems--;
			list.lock.release();

			if (objectCache == NULL) { delete curr; }
			else { objectCache->free(curr); };
			return;
		};
	};

	list.lock.release();
}

template <class T>
T *HeapDoubleList<T>::popFromHead(void)
{
	T		*ret=NULL;
	sListNode	*tmp;

	list.lock.acquire();

	// If removing last item:
	if (list.rsrc.tail == list.rsrc.head) { list.rsrc.tail = NULL; };

	if (list.rsrc.head != NULL)
	{
		tmp = list.rsrc.head;
		if (list.rsrc.head->next != NULL) {
			list.rsrc.head->next->prev = NULL;
		};

		list.rsrc.head = list.rsrc.head->next;
		list.rsrc.nItems--;

		list.lock.release();

		ret = tmp->item;

		if (objectCache == NULL) { delete tmp; }
		else { objectCache->free(tmp); };
		return ret;
	}
	else
	{
		list.lock.release();
		return NULL;
	};
}

template <class T>
T *HeapDoubleList<T>::popFromTail(void)
{
	T		*ret=NULL;
	sListNode	*tmp;

	list.lock.acquire();

	// If removing last item:
	if (list.rsrc.head == list.rsrc.tail) { list.rsrc.head = NULL; };

	if (list.rsrc.tail != NULL)
	{
		tmp = list.rsrc.tail;
		if (list.rsrc.tail->prev != NULL) {
			list.rsrc.tail->prev->next = NULL;
		};

		list.rsrc.tail = list.rsrc.tail->prev;
		list.rsrc.nItems--;

		list.lock.release();

		ret = tmp->item;

		if (objectCache == NULL) { delete tmp; }
		else { objectCache->free(tmp); };
		return ret;
	}
	else
	{
		list.lock.release();
		return NULL;
	};
}

template <class T>
T *HeapDoubleList<T>::getHead(void)
{
	T	*ret=NULL;

	list.lock.acquire();

	if (list.rsrc.head != NULL) {
		ret = list.rsrc.head->item;
	};

	list.lock.release();

	return ret;
}

template <class T>
T *HeapDoubleList<T>::getTail(void)
{
	T	*ret=NULL;

	list.lock.acquire();

	if (list.rsrc.tail != NULL) {
		ret = list.rsrc.tail->item;
	};

	list.lock.release();

	return ret;
}

#endif

