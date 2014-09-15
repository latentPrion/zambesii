#ifndef _HARDWARE_ID_LIST_H
	#define _HARDWARE_ID_LIST_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/processId.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/multipleReaderLock.h>

#define HWIDLIST			"HwId List: "

#define HWIDLIST_INDEX_INVALID		(-1)

#define HWIDLIST_FLAGS_INDEX_VALID	(1<<0)

class HardwareIdList
{
public:
	HardwareIdList(void);

	error_t initialize(
		void *preallocatedMem=NULL, ubit16 preallocatedSize=0);

public:
	class Iterator
	{
	friend class HardwareIdList;
		Iterator(HardwareIdList *list)
		:
		cursor(HWIDLIST_INDEX_INVALID), list(list), currItem(NULL)
		{}

	public:
		Iterator(void)
		:
		cursor(HWIDLIST_INDEX_INVALID), list(NULL), currItem(NULL)
		{}

		void operator ++(void)
			{ currItem = list->getNextItem(&cursor); }

		void *operator *(void)
			{ return currItem; }

		int operator ==(Iterator it)
			{ return currItem == it.currItem; }

		int operator !=(Iterator it)
			{ return currItem != it.currItem; }

		// Allow "cursor" to be read.
		sarch_t		cursor;
	private:
		HardwareIdList	*list;
		void		*currItem;
	};

	// Retrieves an item's pointer by its hardware ID.
	void *getItem(sarch_t id);

	error_t addItem(sarch_t id, void *item);
	void removeItem(sarch_t id);

	// Custom just for the process list.
	error_t findFreeIndex(uarch_t *id);
	uarch_t	getNIndexes(void)
	{
		return static_cast<uarch_t>( arr.rsrc.maxIndex + 1 );
	};

	Iterator begin(void)
	{
		Iterator	it(this);

		++it;
		return it;
	}

	Iterator end(void)
	{
		Iterator	nullIt;

		return nullIt;
	}

	void dump(void);

public:
	struct sArrayNode
	{
		sarch_t		next;
		uarch_t		flags;
		void		*item;
	};

private:
	void *getNextItem(sarch_t *id);

	ubit8		preAllocated;

	struct sArrayState
	{
		sArrayNode	*arr;
		/**	EXPLANATION:
		 * maxIndex: Current last valid (occupied) index in the array.
		 * firstValidIndex: current first valid index in the array.
		 * maxAllocatedIndex: Tracks the size of the array in memory
		 *	consumption, since the array can be a state such that it
		 *	is larger than the maxIndex element indicates it is
		 *	due to unused indexes existing beyond the last currently
		 *	occupied index.
		 **/
		sarch_t		maxIndex, maxAllocatedIndex, firstValidIndex;
	};
	SharedResourceGroup<MultipleReaderLock, sArrayState>	arr;
};

#endif

