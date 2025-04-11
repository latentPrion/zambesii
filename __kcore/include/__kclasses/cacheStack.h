#ifndef _CACHE_STACK_H
	#define _CACHE_STACK_H

	#include <arch/paging.h>
	#include <arch/paddr_t.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kclib/string.h>
	#include <__kclasses/memBmp.h>
	#include <kernel/common/waitLock.h>
	#include <kernel/common/sharedResourceGroup.h>

// This is the number of pages used for each instance of CacheStack.
#define CACHESTACK_NPAGES_PER_STACK		(1)
#define CACHESTACK_MAX_NITEMS(__t)		\
	((PAGING_BASE_SIZE *CACHESTACK_NPAGES_PER_STACK) / sizeof(__t))
#define CACHESTACK_FULL(__t)			(CACHESTACK_MAX_NITEMS(__t) -1)

#define CACHESTACK_EMPTY			(-1)

template <class T>
class CacheStack
{
public:
	CacheStack(void);

public:
	error_t push(T item);
	error_t pop(T *item);

	void flush(MemoryBmp *bmp);

public:
	uarch_t	stackSize;

private:
	struct sStackState
	{
		T		stack[CACHESTACK_MAX_NITEMS(T)];
		int		stackPtr;
	};
	SharedResourceGroup<WaitLock, sStackState>	stack;
};


/** Template Definition
 *****************************************************************************/

template <class T>
CacheStack<T>::CacheStack(void)
:
stack(CC"CacheStack stack")
{
	memset(
		stack.rsrc.stack, 0,
		PAGING_BASE_SIZE * CACHESTACK_NPAGES_PER_STACK);

	stack.rsrc.stackPtr = CACHESTACK_EMPTY;
}

template <class T>
error_t CacheStack<T>::push(T item)
{
	stack.lock.acquire();

	if (stack.rsrc.stackPtr == CACHESTACK_FULL(T))
	{
		stack.lock.release();
		return ERROR_GENERAL;
	};

	stack.rsrc.stackPtr++;
	stack.rsrc.stack[stack.rsrc.stackPtr] = item;

	stack.lock.release();
	return ERROR_SUCCESS;
}

template <class T>
error_t CacheStack<T>::pop(T *item)
{
	stack.lock.acquire();

	if (stack.rsrc.stackPtr == CACHESTACK_EMPTY)
	{
		stack.lock.release();
		return ERROR_GENERAL;
	};

	*item = stack.rsrc.stack[ stack.rsrc.stackPtr ];
	stack.rsrc.stackPtr--;

	stack.lock.release();
	return ERROR_SUCCESS;
}

template <class T>
void CacheStack<T>::flush(MemoryBmp *bmp)
{
	stack.lock.acquire();

	// Can only be frames from the BMP passed as an argument.
	for (; stack.rsrc.stackPtr != CACHESTACK_EMPTY; stack.rsrc.stackPtr--)
	{
		bmp->releaseFrames(
			stack.rsrc.stack[stack.rsrc.stackPtr], stackSize);
	};

	stack.lock.release();
}

#endif

