#ifndef ___KEQUALIZER_LIST_H
	#define ___KEQUALIZER_LIST_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kcxxlib/new>
	#include <__kclasses/__kpageBlock.h>
	#include <__kclasses/debugPipe.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>
	#include <kernel/common/memoryTrib/rawMemAlloc.h>

/**	EXPLANATION:
 * The Kernel Equalizer List provides a form of light object allocation
 * to the most integral classes in the kernel in the absence of dynamic memory
 * allocation.
 *
 * It must only be used to allocate items of (size < PAGING_BASE_SIZE).
 * Optimally, much smaller than the paging base size. The class uses C++ object
 * orientation features, specfically operator overloading, to define the right
 * operators to manipulate the class it is suppposed to allocate.
 *
 * It manages all of the objects as contiguous items in a list of arrays of
 * objects of that type. The objects are sorted within their blocks. That is,
 * each block in the list of blocks holds N entries which are contiguous
 * objects, and within each block, the objects are sorted relative to one
 * another *within* _that_ block.
 *
 * Objects in block B are not sorted relative to block C. Therefore in order
 * to search the list, you would traverse the blocks and search each block
 * until you encounter an object of greater value than the one you want; at
 * such a point, you would move on to the next block and search its object until
 * you find your target object.
 *
 * This class works on page sized chunks and breaks them down, using absolutely
 * no extra memory on each object. However, in order to use this class, several
 * criteria must be met:
 *	* The object type you wish to allocate must be < PAGING_BASE_SIZE.
 *	* The class of the object must have the following operators defined:
 *	  ==, !=, =, >, <, <=, >=.
 *	  Additionally, the following operators must have an 'int' target
 *	  overload form:
 *	  ==, !=, =;
 **/

template <class T>
class __kequalizerListC
{
public:
	__kequalizerListC(void);
	error_t initialize(void) { return ERROR_SUCCESS; }
	~__kequalizerListC(void);

public:
	T *addEntry(T *item);
	T *find(T *item);
	void removeEntry(T *item);

	void dump(void);

private:
	__kpageBLock<T> *findInsertionBlock(void);
	void findInsertionEntry(
		__kpageBLock<T> *block, T *item, uarch_t *entry);

	__kpageBLock<T> *findDeletionEntry(T *item, uarch_t *entry);
	__kpageBLock<T> *getNewBlock(__kpageBLock<T> *joinTo);
	void deleteBlock(__kpageBLock<T> *block);

private:
	sharedResourceGroupC<waitLockC, __kpageBLock<T> *>	head;
};


/** Template Definition
 ****************************************************************************/

template <class T>
__kequalizerListC<T>::__kequalizerListC(void)
{
	head.rsrc = NULL;
}

template <class T>
__kequalizerListC<T>::~__kequalizerListC(void)
{
	__kpageBLock<T>	*tmp;

	while (head.rsrc != NULL)
	{
		tmp = head.rsrc;
		head.rsrc = head.rsrc->header.next;
		rawMemFree(tmp, 1);
	};
}

template <class T>
void __kequalizerListC<T>::dump(void)
{
	printf(NOTICE"eqList: @0x%p, head 0x%p, %d entries per block.\n",
		this, head.rsrc, PAGEBLOCK_NENTRIES(T));

	head.lock.acquire();

	for (__kpageBLock<T> *tmp=head.rsrc;
		tmp != NULL;
		tmp = tmp->header.next)
	{
		uarch_t		nEntries;
		for (nEntries=0; tmp->entries[nEntries] != 0; nEntries++) {};
		printf(CC"\teqList: block 0x%p, %d entries, %d free entries."
			"\n", tmp, nEntries, tmp->header.nFreeEntries);
	};

	head.lock.release();
}

template <class T>
T *__kequalizerListC<T>::addEntry(T *item)
{
	__kpageBLock<T>	*insertionBlock;
	uarch_t			insEntry=0, copyStart;

	if (item == NULL) {
		return NULL;
	};

	head.lock.acquire();

	insertionBlock = findInsertionBlock();
	if (insertionBlock == NULL)
	{
		insertionBlock = getNewBlock(head.rsrc);
		if (insertionBlock == NULL)
		{
			head.lock.release();
			return NULL;
		};

		// Link the new block to the rest of the list.
		head.rsrc = insertionBlock;
	};

	findInsertionEntry(insertionBlock, item, &insEntry);

	for (copyStart = insEntry; copyStart < PAGEBLOCK_NENTRIES(T);
		copyStart++)
	{
		if (insertionBlock->entries[copyStart] == 0) {
			break;
		};
	};

	for (; copyStart > insEntry; copyStart--)
	{
		insertionBlock->entries[copyStart] =
			insertionBlock->entries[copyStart-1];
	};

	// At this point the list is clear for a new entry.
	insertionBlock->entries[insEntry] = *item;
	insertionBlock->header.nFreeEntries--;

	head.lock.release();
	return &insertionBlock->entries[insEntry];
}

template <class T>
T *__kequalizerListC<T>::find(T *item)
{
	if (item == NULL) {
		return NULL;
	};

	head.lock.acquire();

	// Just run through every block and search for the item.
	for (__kpageBLock<T> *current = head.rsrc;
		current != NULL;
		current = current->header.next)
	{
		for (uarch_t i=0;
			(i<PAGEBLOCK_NENTRIES(T)) && 
				(current->entries[i] != 0) &&
				(current->entries[i] <= *item); 
			i++)
		{
			if (current->entries[i] == *item)
			{
				head.lock.release();
				return &current->entries[i];
			};
		};
	};

	head.lock.release();
	return NULL;
}

// fine.
template <class T>
void __kequalizerListC<T>::removeEntry(T *item)
{
	__kpageBLock<T>	*block;
	uarch_t			entry;

	if (item == NULL) {
		return;
	};

	head.lock.acquire();

	block = findDeletionEntry(item, &entry);
	if (block == NULL)
	{
		head.lock.release();
		return;
	};

	// Now we just move things around a bit.
	const uarch_t		nOccupiedEntries =
		PAGEBLOCK_NENTRIES(T) - block->header.nFreeEntries;

	for (; entry < (nOccupiedEntries - 1); entry++) {
		block->entries[entry] = block->entries[entry+1];
	};

	block->entries[nOccupiedEntries - 1] = 0;
	block->header.nFreeEntries++;

	head.lock.release();
}

// The lock must already be held before calling this.
template <class T>
__kpageBLock<T> *__kequalizerListC<T>::findInsertionBlock(void)
{
	for (__kpageBLock<T> *current = head.rsrc;
		current != NULL;
		current = current->header.next)
	{
		if (current->header.nFreeEntries > 0) {
			return current;
		};
	};

	return NULL;
}

// The lock must already be held before calling this.
template <class T>
void __kequalizerListC<T>::findInsertionEntry(
	__kpageBLock<T> *block, T *item, uarch_t *entry
	)
{
	for (uarch_t i=0; i<PAGEBLOCK_NENTRIES(T); i++)
	{
		if (block->entries[i] >= *item || block->entries[i] == 0)
		{
			*entry = i;
			return;
		};
	};
}

// The lock must already be held before calling this.
template <class T>
__kpageBLock<T> *__kequalizerListC<T>::findDeletionEntry(
	T *item, uarch_t *entry
	)
{
	for (__kpageBLock<T> *current = head.rsrc; current != NULL;
		current = current->header.next)
	{
		uarch_t			i;
		const uarch_t		nOccupiedEntries =
			PAGEBLOCK_NENTRIES(T) - current->header.nFreeEntries;

		// Run through until we reach one that is >= 'item', or is "0".
		for (i=0;
			i<nOccupiedEntries && current->entries[i] != 0
				&& (*item > current->entries[i]);
			i++)
		{};

		// If the one we stopped on == 'item', then we found it.
		if (current->entries[i] != 0 && current->entries[i] == *item)
		{
			*entry = i;
			return current;
		};
	};

	return NULL;
}

// The lock must already be held before calling this.
template <class T>
__kpageBLock<T> *__kequalizerListC<T>::getNewBlock(__kpageBLock<T> *joinTo)
{
	__kpageBLock<T>	*ret;

	ret = new (rawMemAlloc(1, 0)) __kpageBLock<T>;
	if (ret == NULL) {
		return NULL;
	};

	ret->header.nFreeEntries = PAGEBLOCK_NENTRIES(T);
	ret->header.next = joinTo;

	for (uarch_t i=0; i<PAGEBLOCK_NENTRIES(T); i++) {
		ret->entries[i] = 0;
	};

	return ret;
}

#endif

