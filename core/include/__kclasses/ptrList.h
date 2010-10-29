#ifndef _POINTER_LINKED_LIST_H
	#define _POINTER_LINKED_LIST_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kflagManipulation.h>
	#include <__kstdlib/__kcxxlib/new>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

#define PTRLIST_FLAGS_NO_AUTOLOCK	(1<<0)

template <class T>
class ptrListC
{
public:
	ptrListC(void);
	~ptrListC(void);

public:
	error_t insert(T *item);
	void remove(T *item);

	ubit32 getNItems(void)
	{
		ubit32		ret;

		head.lock.acquire();
		ret = head.rsrc.nItems;
		head.lock.release();

		return ret;
	};

	// T *getNextItem(void **const handle, ubit32 flags=0);
	// T *getItem(ubit32 num);

	void lock(void);
	void unlock(void);

private:
	struct ptrListNodeS
	{
		T		*item;
		ptrListNodeS	*next;
	};
	struct ptrListStateS
	{
		ptrListNodeS	*ptr;
		ubit32		nItems;
	};

	sharedResourceGroupC<waitLockC, ptrListStateS *>	head;
};


/**	Template Definition
 ******************************************************************************/

template <class T>
ptrListC<T>::ptrListC(void)
{
	head.rsrc.nItems = 0;
	head.rsrc.ptr = __KNULL;
};

template <class T>
ptrListC<T>::~ptrListC(void)
{
	ptrListNodeS		*cur, *tmp;

	for (cur = head.rsrc.ptr; cur != __KNULL; )
	{
		tmp = cur;
		cur = cur->next;
		delete tmp;
	};
}

template <class T>
error_t ptrListC<T>::insert(T *item)
{
	ptrListNodeS		*node;

	node = new ptrListNodeS;
	if (node == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};
	node->item = item;

	head.lock.acquire();

	node->next = head.rsrc.ptr;
	head.rsrc.ptr = node;

	head.rsrc.nItems++;

	head.lock.release();
	return ERROR_SUCCESS;
}

template <class T>
void ptrListC<T>::remove(T *item)
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
			delete tmp;
			return;
		};
		prev = cur;
		cur = cur->next;
	};

	head.lock.release();
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
	
#endif

