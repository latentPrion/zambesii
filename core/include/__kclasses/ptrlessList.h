#ifndef _POINTERLESS_LIST_H
	#define _POINTERLESS_LIST_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/debugPipe.h>
	#include <kernel/common/sharedResourceGroup.h>

	/**	EXPLANATION:
	 * Singly linked list class that doesn't have any memory overhead; the
	 * objects stored in the list are expected to have a header inside of
	 * them at offset 0. This header is defined by the list class.
	 *
	 * The header contains the "next" pointer to address the next item in
	 * the list. Thus the overhead of allocating memory specifically for
	 * pointing to the next item and the current one is eliminated. Not very
	 * innovative, very routine.
	 *
	 * List is FIFO.
	 *
	 * The object class to be used with this list must have the header
	 * inside of itself, with the member name "listHeader".
	 **/

template <class T>
class ptrlessListC
{
public:
	ptrlessListC(void);
	error_t initialize(void);

public:
	// Inserts an element at the front of the list.
	void insert(T *item);
	// Removes the item at the end of the list and returns its pointer.
	T *pop(void);
	// Returns a pointer to the Nth item.
	T *getItem(ubit32 num);

	ubit32 getNItems(void)
	{
		ubit32		ret;

		list.lock.acquire();
		ret = list.rsrc.nItems;
		list.lock.release();
		return ret;
	};

	void dump(void);

public:
	struct headerS
	{
		T	*next;
	};

private:
	struct listStateS
	{
		T		*head, *tail;
		ubit32		nItems;
	};

	sharedResourceGroupC<waitLockC, listStateS>	list;
};


/**	Template Definition
 ******************************************************************************/

template <class T> ptrlessListC<T>::ptrlessListC(void)
{
	list.rsrc.head = list.rsrc.tail = __KNULL;
	list.rsrc.nItems = 0;
}

template <class T> error_t ptrlessListC<T>::initialize(void)
{
	return ERROR_SUCCESS;
}

template <class T> void ptrlessListC<T>::insert(T *item)
{
	item->listHeader.next = __KNULL;

	list.lock.acquire();

	if (list.rsrc.tail == __KNULL) {
		list.rsrc.head = list.rsrc.tail = item;
	}
	else
	{
		list.rsrc.tail->listHeader.next = item;
		list.rsrc.tail = item;
	};

	list.rsrc.nItems++;

	list.lock.release();
}

template <class T> T *ptrlessListC<T>::pop(void)
{
	T	*ret;

	list.lock.acquire();

	if (list.rsrc.head == __KNULL)
	{
		list.lock.release();
		return __KNULL;
	};

	if (list.rsrc.tail == list.rsrc.head) {
		list.rsrc.tail = __KNULL;
	};

	ret = list.rsrc.head;
	list.rsrc.head = list.rsrc.head->listHeader.next;
	list.rsrc.nItems--;

	list.lock.release();

	return ret;
}

template <class T> void ptrlessListC<T>::dump(void)
{

	ubit32		count=4;

	list.lock.acquire();

	__kprintf(NOTICE"PointerlessList @ 0x%p: dumping; lock @ 0x%p.\n"
		"\tNum items: %d, head: 0x%p, tail 0x%p.\n\t",
		this, &list.lock,
		list.rsrc.nItems, list.rsrc.head, list.rsrc.tail);

	for (T *curr = list.rsrc.head;
		curr != __KNULL;
		curr = curr->listHeader.next, count--)
	{
		if (count == 0) { __kprintf(CC"\n\t"); count = 4; };
		__kprintf(CC"obj: 0x%p, ", curr);
	};
	__kprintf(CC"\n");

	list.lock.release();
}

template <class T> T *ptrlessListC<T>::getItem(ubit32 num)
{
	T		*curr;

	list.lock.acquire();

	for (curr = list.rsrc.head;
		curr != __KNULL && num > 0;
		curr = curr->listHeader.next, num--)
	{};

	list.lock.release();

	return curr;
}

#endif

