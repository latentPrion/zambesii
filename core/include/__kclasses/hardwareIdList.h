#ifndef _HARDWARE_ID_LIST_H
	#define _HARDWARE_ID_LIST_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/processId.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/multipleReaderLock.h>

#define HWIDLIST			"HwId List: "

#define HWIDLIST_INDEX_INVALID		(-1)

#define HWIDLIST_FLAGS_INDEX_VALID	(1<<0)

class hardwareIdListC
{
public:
	hardwareIdListC(void);
	void __kspaceSetState(sarch_t id, void *arrayMem);
public:
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

	/* Allows a caller to loop through the array without knowing about
	 * the layout of the members.
	 *
	 *	USAGE:
	 * Prepare an iterator of type sarch_t. Call prepareForLoop(), saving
	 * the return value in the iterator. The returned value is the ID of the
	 * first valid (occupied) index in the array. From then on, call
	 * getLoopItem(&it) with the address of the iterator as shown to read
	 * the current value of the index in the iterator and advance it to the
	 * next valid index for reading.
	 *
	 * Repeat until getLoopItem() returns __KNULL.
	 **/
	sarch_t prepareForLoop(void);
	void *getLoopItem(sarch_t *id);

	void dump(void);

public:
	struct arrayNodeS
	{
		sarch_t		next;
		uarch_t		flags;
		void		*item;
	};

private:
	struct arrayStateS
	{
		arrayNodeS	*arr;
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
	sharedResourceGroupC<multipleReaderLockC, arrayStateS>	arr;
};

#endif

