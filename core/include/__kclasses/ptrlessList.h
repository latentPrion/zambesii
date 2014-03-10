#ifndef _POINTERLESS_LIST_H
	#define _POINTERLESS_LIST_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kflagManipulation.h>
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
	const static ubit32		OP_FLAGS_UNLOCKED=(1<<0);

	// Inserts an element at the front of the list.
	void insert(T *item, ubit32 flags=0);
	// Requires:
	void sortedInsert(T *item, ubit32 flags=0);
	sarch_t remove(T *item);
	// Removes the item at the end of the list and returns its pointer.
	T *pop(void);
	// Returns a pointer to the Nth item.
	T *getItem(ubit32 num) const;
	sbit8 find(T *item, ubit32 flags=0);

	// Iterator methods.
	void lock(void) { list.lock.acquire(); }
	void unlock(void) { list.lock.release(); }

	class iteratorC
	{
	friend class ptrlessListC;
	public:
		iteratorC(void) : list(NULL), currItem(NULL) {}

		void operator ++(void)
		{
			if (currItem == NULL) { return; };
			currItem = currItem->listHeader.next;
		}

		T *operator *(void) const
		{
			return currItem;
		}

		int operator !=(ptrlessListC<T>::iteratorC it)
		{
			if (it.currItem == NULL)
			{
				if (currItem == NULL) { return 0; };
				return 1;
			};

			return 0;
		}

		int operator ==(ptrlessListC<T>::iteratorC it)
		{
			if (it.currItem == NULL)
			{
				if (currItem == NULL) { return 1; };
				return 0;
			};

			return 0;
		}

	private:
		ptrlessListC<T>		*list;
		T			*currItem;
	};

	iteratorC begin(void)
	{
		iteratorC	it;

		it.list = this;
		it.currItem = list.rsrc.head;
		return it;
	}

	iteratorC end(void)
	{
		iteratorC	it;

		it.list = this;
		it.currItem = NULL;
		return it;
	}

	ubit32 getNItems(ubit32 flags=0)
	{
		ubit32		ret;

		if (!FLAG_TEST(flags, OP_FLAGS_UNLOCKED)) { lock(); };
		ret = list.rsrc.nItems;
		if (!FLAG_TEST(flags, OP_FLAGS_UNLOCKED)) { unlock(); };
		return ret;
	};

	void dump(void);

public:
	struct headerS
	{
		friend class ptrlessListC;
		void setNextItem(T *next) { this->next = next; }
		T *getNextItem(void) const { return this->next; }

	private:
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
	list.rsrc.head = list.rsrc.tail = NULL;
	list.rsrc.nItems = 0;
}

template <class T> error_t ptrlessListC<T>::initialize(void)
{
	return ERROR_SUCCESS;
}

template <class T> void ptrlessListC<T>::insert(T *item, ubit32 flags)
{
	item->listHeader.next = NULL;

	if (!FLAG_TEST(flags, OP_FLAGS_UNLOCKED)) { lock(); };

	if (list.rsrc.tail == NULL) {
		list.rsrc.head = list.rsrc.tail = item;
	}
	else
	{
		list.rsrc.tail->listHeader.next = item;
		list.rsrc.tail = item;
	};

	list.rsrc.nItems++;

	if (!FLAG_TEST(flags, OP_FLAGS_UNLOCKED)) { unlock(); };
}

template <class T>
void ptrlessListC<T>::sortedInsert(T *item, ubit32 flags)
{
	iteratorC	curr, prev;

	item->listHeader.next = NULL;

	if (!FLAG_TEST(flags, OP_FLAGS_UNLOCKED)) { lock(); };

	if (list.rsrc.tail == NULL)
	{
		list.rsrc.tail = list.rsrc.head = item;
		list.rsrc.nItems++;

		if (!FLAG_TEST(flags, OP_FLAGS_UNLOCKED)) { unlock(); };
		return;
	};

	prev = end();
	for (curr=begin(); curr != end(); prev=curr, ++curr)
	{
		// For duplicates, we insert it the same way.
		if (item <= *curr)
		{
			item->listHeader.setNextItem(*curr);
			if (prev == end()) {
				list.rsrc.head = item;
			} else {
				(*prev)->listHeader.setNextItem(item);
			};

			list.rsrc.nItems++;
			if (!FLAG_TEST(flags, OP_FLAGS_UNLOCKED)) { unlock(); };
			return;
		};
	};

	// If we got all the way here, the item belongs at the end of the list.
	list.rsrc.tail->listHeader.setNextItem(item);
	list.rsrc.tail = item;
	list.rsrc.nItems++;

	if (!FLAG_TEST(flags, OP_FLAGS_UNLOCKED)) { unlock(); };
}

template <class T> sarch_t ptrlessListC<T>::remove(T *item)
{
	iteratorC	it, prev;

	lock();

	for (it = begin(); it != end(); prev = it, ++it)
	{
		if (*it != item) { continue; };

		// If target item is the only one in the list:
		if (list.rsrc.head == list.rsrc.tail) {
			list.rsrc.head = list.rsrc.tail = NULL;
		}
		// If target item is first in the list, advance head pointer.
		else if (prev == end()) {
			list.rsrc.head = list.rsrc.head->listHeader.next;
		}
		else
		{
			if (item == list.rsrc.tail) {
				list.rsrc.tail = *prev;
			};

			(*prev)->listHeader.setNextItem(
				(*it)->listHeader.getNextItem());
		};

		list.rsrc.nItems--;
		unlock();
		return 1;
	}

	unlock();
	return 0;
}

template <class T> T *ptrlessListC<T>::pop(void)
{
	T	*ret;

	list.lock.acquire();

	if (list.rsrc.head == NULL)
	{
		list.lock.release();
		return NULL;
	};

	if (list.rsrc.tail == list.rsrc.head) {
		list.rsrc.tail = NULL;
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

	printf(NOTICE"PointerlessList @ 0x%p: dumping; lock @ 0x%p.\n"
		"\tNum items: %d, head: 0x%p, tail 0x%p.\n\t",
		this, &list.lock,
		list.rsrc.nItems, list.rsrc.head, list.rsrc.tail);

	for (T *curr = list.rsrc.head;
		curr != NULL;
		curr = curr->listHeader.next, count--)
	{
		if (count == 0) { printf(CC"\n\t"); count = 4; };
		printf(CC"obj: 0x%p, ", curr);
	};
	printf(CC"\n");

	list.lock.release();
}

template <class T> T *ptrlessListC<T>::getItem(ubit32 num) const
{
	T		*curr;

	list.lock.acquire();

	for (curr = list.rsrc.head;
		curr != NULL && num > 0;
		curr = curr->listHeader.next, num--)
	{};

	list.lock.release();

	return curr;
}

template <class T>
sbit8 ptrlessListC<T>::find(T *item, ubit32 flags)
{
	iteratorC	it;
	sbit8		ret=0;

	if (!FLAG_TEST(flags, OP_FLAGS_UNLOCKED)) { list.lock.acquire(); };

	for (it=begin(); it != end(); ++it)
	{
		T		*currItem;

		currItem = *it;
		if (currItem != item) { continue; };
		ret = 1;
		break;
	};

	if (!FLAG_TEST(flags, OP_FLAGS_UNLOCKED)) { list.lock.release(); };
	return ret;
}

#endif

