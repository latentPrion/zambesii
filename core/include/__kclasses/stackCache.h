#ifndef _STACK_CACHE_H
	#define _STACK_CACHE_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/cacheStack.h>

#define STACKCACHE_NSTACKS		1
// The first stack in the cache stores page blocks of size 1 page large.
#define STACKCACHE_STACK1_SIZE		1

class NumaMemoryBank;

/**	EXPLANATION:
 * StackCache is a simple class which acts as a caching mechanism for items of
 * integral type.
 *
 * Its use is confined to the Memory Stream for the caching of virtual addresses
 * within a process. It caches single pages only. The whole idea is to speed up
 * common allocation sizes (such as single pages), so unless a particular number
 * of contiguous pages becomes a very common allocation, and can be so proven,
 * it cannot be placed here.
 *
 * To change the number of pages each cached page size takes up, modify
 * 'CACHESTACK_NPAGES_PER_STACK' in cacheStack.h.
 **/

template <class T>
class StackCache
{
friend class NumaMemoryBank;

public:
	StackCache(void);
	error_t initialize(void) { return ERROR_SUCCESS; }

public:
	error_t push(uarch_t category, T item);
	error_t pop(uarch_t category, T *item);

	void flush(MemoryBmp *bmp);

private:
	CacheStack<T>		stacks[STACKCACHE_NSTACKS];
};


/** Template Definition
 ***************************************************************************/

template <class T>
StackCache<T>::StackCache(void)
{
	stacks[0].stackSize = STACKCACHE_STACK1_SIZE;
}

template <class T>
error_t StackCache<T>::push(uarch_t category, T item)
{
	for (sarch_t i=0; i<STACKCACHE_NSTACKS; i++)
	{
		if (stacks[i].stackSize == category) {
			return stacks[i].push(item);
		};
	};
	return ERROR_GENERAL;
}

template <class T>
error_t StackCache<T>::pop(uarch_t category, T *item)
{
	for (sarch_t i=0; i<STACKCACHE_NSTACKS; i++)
	{
		if (stacks[i].stackSize == category) {
			return stacks[i].pop(item);
		};
	};
	return ERROR_GENERAL;
}

template <class T>
void StackCache<T>::flush(MemoryBmp *bmp)
{
	for (sarch_t i=0; i<STACKCACHE_NSTACKS; i++) {
		stacks[i].flush(bmp);
	};
}

#endif

