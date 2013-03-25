#ifndef _SORTED_POINTER_DOUBLY_LINKED_LIST_H
	#define _SORTED_POINTER_DOUBLY_LINKED_LIST_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kcxxlib/new>
	#include <__kclasses/debugPipe.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

#define SRT_PTRDBLLIST			"Sorted-Dbl-Link PtrList: "

#define SRT_PTRDBLLIST_ADD_ASCENDING		0x0
#define SRT_PTRDBLLIST_ADD_DESCENDING		0x1

template <class T, class criterionType>
class sortedPointerDoubleListC
{
public:
	sortedPointerDoubleListC(void) {}
	~sortedPointerDoubleListC(void) {};

	error_t initialize(void) { return ERROR_SUCCESS; };

public:
	void dump(void);

	error_t addItem(
		T *item, criterionType criterion,
		ubit8 mode=SRT_PTRDBLLIST_ADD_ASCENDING);

	void removeItem(T *item);
	T *popFromHead(void);
	T *popFromTail(void);
	T *getHead(void);
	T *getTail(void);

	uarch_t	getNItems(void);

protected:
	struct listNodeS
	{
		criterionType	criterion;
		T		*item;
		listNodeS	*prev, *next;
	};

	struct listStateS
	{
		listStateS(void)
		:
		head(__KNULL), tail(__KNULL), nItems(0)
		{}

		listNodeS	*head, *tail;
		uarch_t		nItems;
	};

	sharedResourceGroupC<waitLockC, listStateS>	list;
};


/**	Template definition.
 ******************************************************************************/

template <class T, class criterionType>
uarch_t sortedPointerDoubleListC<T, criterionType>::getNItems(void)
{
	uarch_t		ret;

	list.lock.acquire();
	ret = list.rsrc.nItems;
	list.lock.release();

	return ret;
}

template <class T, class criterionType>
void sortedPointerDoubleListC<T, criterionType>::dump(void)
{
	listNodeS	*curr, *head, *tail;
	sbit8		flipFlop=0;

	list.lock.acquire();
	head = list.rsrc.head;
	tail = list.rsrc.tail;
	list.lock.release();

	__kprintf(NOTICE SRT_PTRDBLLIST"@ head 0x%p, tail 0x%p, %d items, "
		"lock obj 0x%p: Dumping.\n",
		head, tail, getNItems(), &list.lock);

	list.lock.acquire();

	curr = head;
	for (; curr != __KNULL; curr = curr->next, flipFlop++)
	{
		if (flipFlop < 4) {
			__kprintf(CC"\t0x%p(%d) ", curr->item, curr->criterion);
		}
		else
		{
			__kprintf(CC"0x%p(%d)\n", curr->item, curr->criterion);
			flipFlop = -1;
		};
	};

	if (flipFlop != 0) { __kprintf(CC"\n"); };
	list.lock.release();
}

template <class T, class criterionType>
error_t sortedPointerDoubleListC<T, criterionType>::addItem(
	T *item, criterionType criterion, ubit8 mode
	)
{
	listNodeS	*newNode, *curr;

	if (mode == SRT_PTRDBLLIST_ADD_DESCENDING)
	{
		__kprintf(ERROR SRT_PTRDBLLIST"addItem(0x%p,desc): "
			"Descended ordering insertion is currently "
			"unimplemented.\n",
			item);

		return ERROR_UNIMPLEMENTED;
	};

	newNode = new listNodeS;
	if (newNode == __KNULL)
	{
		__kprintf(ERROR SRT_PTRDBLLIST"addItem(0x%p,%s): Failed "
			"to alloc mem for new node.\n",
			item,
			(mode == SRT_PTRDBLLIST_ADD_ASCENDING)?"asc":"desc");

		return ERROR_MEMORY_NOMEM;
	};

	newNode->item = item;
	newNode->criterion = criterion;

	list.lock.acquire();

	// If list is empty:
	if (list.rsrc.head == __KNULL)
	{
		list.rsrc.head = list.rsrc.tail = newNode;
		newNode->prev = __KNULL;
		newNode->next = __KNULL;
		list.rsrc.nItems++;

		list.lock.release();
		return ERROR_SUCCESS;
	};

	// If only one item in list:
	if (list.rsrc.head == list.rsrc.tail)
	{
		// For both greater than and equal, insert after.
		if (newNode->criterion >= list.rsrc.head->criterion)
		{
			newNode->prev = list.rsrc.head;
			newNode->next = __KNULL;
			list.rsrc.head->next = newNode;
			list.rsrc.tail = newNode;
		}
		else
		{
			newNode->prev = __KNULL;
			newNode->next = list.rsrc.head;
			list.rsrc.head->prev = newNode;
			list.rsrc.head = newNode;
		};

		list.rsrc.nItems++;
		list.lock.release();
		return ERROR_SUCCESS;
	};

	// If inserting at head of list:
	if (list.rsrc.head->criterion > newNode->criterion)
	{
		newNode->prev = __KNULL;
		newNode->next = list.rsrc.head;
		list.rsrc.head->prev = newNode;
		list.rsrc.head = newNode;
		list.rsrc.nItems++;

		list.lock.release();
		return ERROR_SUCCESS;
	};

	// If inserting at tail of list:
	if (list.rsrc.tail->criterion <= newNode->criterion)
	{
		newNode->prev = list.rsrc.tail;
		newNode->next = __KNULL;
		list.rsrc.tail->next = newNode;
		list.rsrc.tail = newNode;
		list.rsrc.nItems++;

		list.lock.release();
		return ERROR_SUCCESS;
	};

	// Every other case, inserting in the middle of two other nodes.
	for (curr = list.rsrc.head; curr != __KNULL; curr = curr->next)
	{
		if (newNode->criterion >= curr->criterion)
		{
			newNode->prev = curr;
			newNode->next = curr->next;
			curr->next->prev = newNode;
			curr->next = newNode;
			list.rsrc.nItems++;

			list.lock.release();
			return ERROR_SUCCESS;
		};
	};

	return ERROR_UNKNOWN;
}

template <class T, class criterionType>
void sortedPointerDoubleListC<T, criterionType>::removeItem(T *item)
{
	listNodeS	*curr;

	list.lock.acquire();
	// Just run through until we find it, then remove it.
	for (curr = list.rsrc.head; curr != __KNULL; curr = curr->next)
	{
		if (curr->item == item)
		{
			if (list.rsrc.head == curr)
			{
				list.rsrc.head = curr->next;
				if (list.rsrc.head != __KNULL) {
					curr->next->prev = __KNULL;
				};
			}
			else { curr->prev->next = curr->next; };

			if (list.rsrc.tail == curr)
			{
				list.rsrc.tail = curr->prev;
				if (list.rsrc.tail != __KNULL) {
					curr->prev->next = __KNULL;
				};
			}
			else { curr->next->prev = curr->prev; };

			list.rsrc.nItems--;
			list.lock.release();

			delete curr;
			return;
		};
	};

	list.lock.release();
}

template <class T, class criterionType>
T *sortedPointerDoubleListC<T, criterionType>::popFromHead(void)
{
	T		*ret=__KNULL;
	listNodeS	*tmp;

	list.lock.acquire();

	// If removing last item:
	if (list.rsrc.tail == list.rsrc.head) { list.rsrc.tail = __KNULL; };

	if (list.rsrc.head != __KNULL)
	{
		tmp = list.rsrc.head;
		if (list.rsrc.head->next != __KNULL) {
			list.rsrc.head->next->prev = __KNULL;
		};

		list.rsrc.head = list.rsrc.head->next;
		list.rsrc.nItems--;

		list.lock.release();

		ret = tmp->item;
		delete tmp;
		return ret;
	}
	else
	{
		list.lock.release();
		return __KNULL;
	};
}

template <class T, class criterionType>
T *sortedPointerDoubleListC<T, criterionType>::popFromTail(void)
{
	T		*ret=__KNULL;
	listNodeS	*tmp;

	list.lock.acquire();

	// If removing last item:
	if (list.rsrc.head == list.rsrc.tail) { list.rsrc.head = __KNULL; };

	if (list.rsrc.tail != __KNULL)
	{
		tmp = list.rsrc.tail;
		if (list.rsrc.tail->prev != __KNULL) {
			list.rsrc.tail->prev->next = __KNULL;
		};

		list.rsrc.tail = list.rsrc.tail->prev;
		list.rsrc.nItems--;

		list.lock.release();

		ret = tmp->item;
		delete tmp;
		return ret;
	}
	else
	{
		list.lock.release();
		return __KNULL;
	};
}

template <class T, class criterionType>
T *sortedPointerDoubleListC<T, criterionType>::getHead(void)
{
	T	*ret=__KNULL;

	list.lock.acquire();

	if (list.rsrc.head != __KNULL) {
		ret = list.rsrc.head->item;
	};

	list.lock.release();

	return ret;
}

template <class T, class criterionType>
T *sortedPointerDoubleListC<T, criterionType>::getTail(void)
{
	T	*ret=__KNULL;

	list.lock.acquire();

	if (list.rsrc.tail != __KNULL) {
		ret = list.rsrc.tail->item;
	};

	list.lock.release();

	return ret;
}

#endif

