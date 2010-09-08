#ifndef _CACHE_STACK_H
	#define _CACHE_STACK_H

	#include <arch/paging.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kclib/string.h>
	#include <kernel/common/waitLock.h>
	#include <kernel/common/sharedResourceGroup.h>

// This is the number of pages used for each instance of cacheStackC.
#define CACHESTACK_NPAGES_PER_STACK		(1)
#define CACHESTACK_MAX_NITEMS(__t)		\
	((PAGING_BASE_SIZE *CACHESTACK_NPAGES_PER_STACK) / sizeof(__t))
#define CACHESTACK_FULL(__t)			(CACHESTACK_MAX_NITEMS(__t) -1)

#define CACHESTACK_EMPTY			(-1)

template <class T>
class cacheStackC
{
public:
	cacheStackC(void);

public:
	error_t push(T item);
	error_t pop(T *item);

public:
	uarch_t	stackSize;

private:
	struct stackStateS
	{
		T		stack[CACHESTACK_MAX_NITEMS(T)];
		int		stackPtr;
	};
	sharedResourceGroupC<waitLockC, stackStateS>	stack;
};


/** Template Definition
 *****************************************************************************/

template <class T>
cacheStackC<T>::cacheStackC(void)
{
	memset(
		stack.rsrc.stack, 0,
		PAGING_BASE_SIZE * CACHESTACK_NPAGES_PER_STACK);

	stack.rsrc.stackPtr = CACHESTACK_EMPTY;
}

template <class T>
error_t cacheStackC<T>::push(T item)
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
error_t cacheStackC<T>::pop(T *item)
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

#endif

