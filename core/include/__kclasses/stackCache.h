#ifndef _STACK_CACHE_H
	#define _STACK_CACHE_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/cacheStack.h>

#define STACKCACHE_NSTACKS		5
#define STACKCACHE_STACK1_SIZE		1
#define STACKCACHE_STACK2_SIZE		2
#define STACKCACHE_STACK3_SIZE		4
#define STACKCACHE_STACK4_SIZE		6
#define STACKCACHE_STACK5_SIZE		8

class numaMemoryBankC;

// T must be an integral type, or must have operator=() defined.
template <class T>
class stackCacheC
{
friend class numaMemoryBankC;

public:
	stackCacheC(void);

public:
	error_t push(uarch_t category, T item);
	error_t pop(uarch_t category, T *item);

private:
	cacheStackC<T>		stacks[STACKCACHE_NSTACKS];
};


/** Template Definition
 ***************************************************************************/

template <class T>
stackCacheC<T>::stackCacheC(void)
{
	stacks[0].stackSize = STACKCACHE_STACK1_SIZE;
	stacks[1].stackSize = STACKCACHE_STACK2_SIZE;
	stacks[2].stackSize = STACKCACHE_STACK3_SIZE;
	stacks[3].stackSize = STACKCACHE_STACK4_SIZE;
	stacks[4].stackSize = STACKCACHE_STACK5_SIZE;
}

template <class T>
error_t stackCacheC<T>::push(uarch_t category, T item)
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
error_t stackCacheC<T>::pop(uarch_t category, T *item)
{
	for (sarch_t i=0; i<STACKCACHE_NSTACKS; i++)
	{
		if (stacks[i].stackSize == category) {
			return stacks[i].pop(item);
		};
	};
	return ERROR_GENERAL;
}

#endif

