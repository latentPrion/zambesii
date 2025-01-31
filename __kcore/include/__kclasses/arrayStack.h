#ifndef _ARRAY_STACK_H
	#define _ARRAY_STACK_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kclib/string.h>
	#include <__kstdlib/__kcxxlib/new>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

#define ARRAYSTACK_IS_EMPTY(__c)		(__c < 0)
#define ARRAYSTACK_IS_FULL(__c,__n)		(__c > (__n - 1))

template <class T>
class ArrayStack
{
public:
	ArrayStack(ubit32 nItems);
	error_t initialize(void);

public:
	error_t push(T);
	error_t push2(T, T);
	error_t pop(T *);
	error_t pop2(T *, T *);

private:
	struct sStackState
	{
		T	*arr;
		sbit32	cursor;
	};
	SharedResourceGroup<WaitLock, sStackState>	stack;
	ubit32	nItems;
};


/**	Template Definition
 *****************************************************************************/

template <class T>
ArrayStack<T>::ArrayStack(ubit32 nItems)
{
	stack.rsrc.arr = NULL;
	stack.rsrc.cursor = -1;
	ArrayStack::nItems = nItems;
}

template <class T>
error_t ArrayStack<T>::initialize(void)
{
	stack.rsrc.arr = new T[nItems];
	if (stack.rsrc.arr == NULL) {
		return ERROR_MEMORY_NOMEM;
	};
	return ERROR_SUCCESS;
}

template <class T>
error_t ArrayStack<T>::push(T item)
{
	stack.lock.acquire();

	if (ARRAYSTACK_IS_FULL(
		stack.rsrc.cursor, static_cast<sbit32>( nItems )))
	{
		stack.lock.release();
		return ERROR_GENERAL;
	};

	stack.rsrc.cursor++;
	stack.rsrc.arr[stack.rsrc.cursor] = item;

	stack.lock.release();
	return ERROR_SUCCESS;
}

template <class T>
error_t ArrayStack<T>::push2(T item, T item2)
{
	stack.lock.acquire();

	if (ARRAYSTACK_IS_FULL(
		stack.rsrc.cursor + 2, static_cast<sbit32>( nItems )))
	{
		stack.lock.release();
		return ERROR_GENERAL;
	};

	stack.rsrc.cursor++;
	stack.rsrc.arr[stack.rsrc.cursor] = item;
	stack.rsrc.cursor++;
	stack.rsrc.arr[stack.rsrc.cursor] = item2;

	stack.lock.release();
	return ERROR_SUCCESS;
}

template <class T>
error_t ArrayStack<T>::pop(T *item)
{
	stack.lock.acquire();

	if (ARRAYSTACK_IS_EMPTY(stack.rsrc.cursor))
	{
		stack.lock.release();
		return ERROR_GENERAL;
	};

	*item = stack.rsrc.arr[stack.rsrc.cursor];
	stack.rsrc.cursor--;

	stack.lock.release();
	return ERROR_SUCCESS;
}

template <class T>
error_t ArrayStack<T>::pop2(T *item1, T *item2)
{
	stack.lock.acquire();

	if (ARRAYSTACK_IS_EMPTY(stack.rsrc.cursor - 2))
	{
		stack.lock.release();
		return ERROR_GENERAL;
	};

	*item2 = stack.rsrc.arr[stack.rsrc.cursor];
	stack.rsrc.cursor--;
	*item1 = stack.rsrc.arr[stack.rsrc.cursor];
	stack.rsrc.cursor--;

	stack.lock.release();
	return ERROR_SUCCESS;
}

#endif

