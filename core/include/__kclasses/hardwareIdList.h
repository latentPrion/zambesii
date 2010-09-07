#ifndef _HARDWARE_ID_LIST_H
	#define _HARDWARE_ID_LIST_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kflagManipulation.h>
	#include <__kstdlib/__kclib/string.h>
	#include <__kstdlib/__kcxxlib/new>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/multipleReaderLock.h>

#define HWIDLIST_INDEX_INVALID		(-1)

#define HWIDLIST_FLAGS_INDEX_VALID	(1<<0)

template <class T>
class hardwareIdListC
{
public:
	hardwareIdListC(void);
	void __kspaceSetState(void *arrayMem);
public:
	// Retrieves an item's pointer by its hardware ID.
	T *getItem(sarch_t id);

	error_t addItem(sarch_t id, T *item);
	void removeItem(sarch_t id);

	/* Allows a caller to loop through the array without knowing about
	 * the layout of the members.
	 **/
	sarch_t prepareForLoop(void);
	T *getLoopItem(sarch_t *id);

private:
	struct arrayNodeS
	{
		sarch_t		next;
		uarch_t		flags;
		T		*item;
	};
	struct arrayStateS
	{
		arrayNodeS	*arr;
		sarch_t		maxIndex, firstValidIndex;
	};
	sharedResourceGroupC<multipleReaderLockC, arrayStateS>	arr;
};


/**	Template Definition.
 *****************************************************************************/

template <class T>
hardwareIdListC<T>::hardwareIdListC(void)
{
	arr.rsrc.maxIndex = HWIDLIST_INDEX_INVALID;
	arr.rsrc.firstValidIndex = HWIDLIST_INDEX_INVALID;
	arr.rsrc.arr = __KNULL;
}

template <class T>
void hardwareIdListC<T>::__kspaceSetState(void *arrayMem)
{
	arr.rsrc.arr = static_cast<arrayNodeS *>( arrayMem );
	arr.rsrc.maxIndex = 0;
}	

template <class T>
T *hardwareIdListC<T>::getItem(sarch_t id)
{
	uarch_t		rwFlags;
	T		*ret=__KNULL;

	arr.lock.readAcquire(&rwFlags);

	if (id > arr.rsrc.maxIndex)
	{
		arr.lock.readRelease(rwFlags);
		return __KNULL;
	};

	if (__KFLAG_TEST(arr.rsrc.arr[id].flags, HWIDLIST_FLAGS_INDEX_VALID)) {
		ret = arr.rsrc.arr[id].item;
	};

	arr.lock.readRelease(rwFlags);
	return ret;
}

template <class T>
sarch_t hardwareIdListC<T>::prepareForLoop(void)
{
	uarch_t		rwFlags;
	sarch_t		ret;

	arr.lock.readAcquire(&rwFlags);
	ret = arr.rsrc.firstValidIndex;
	arr.lock.readRelease(rwFlags);

	return ret;
}

template <class T>
T *hardwareIdListC<T>::getLoopItem(sarch_t *context)
{
	uarch_t		rwFlags;
	T		*ret;

	// 'context' is an index into the array.
	arr.lock.readAcquire(&rwFlags);

	if (!__KFLAG_TEST(arr.rsrc.arr[*context].flags, HWIDLIST_FLAGS_INDEX_VALID))
	{
		arr.lock.readRelease(rwFlags);
		return __KNULL;
	};

	ret = arr.rsrc.arr[*context].item;
	*context = arr.rsrc.arr[*context].next;

	arr.lock.readRelease(rwFlags);
	return ret;
}

template <class T>
error_t hardwareIdListC<T>::addItem(sarch_t id, T *item)
{
	uarch_t		rwFlags;
	sarch_t		maxIndex, itemNextIndex=HWIDLIST_INDEX_INVALID;
	arrayNodeS	*tmp, *old;

	if (item == __KNULL) {
		return ERROR_INVALID_ARG_VAL;
	};

	// See if there are already enough indexes to hold the new item.
	arr.lock.readAcquire(&rwFlags);
	maxIndex = arr.rsrc.maxIndex;
	arr.lock.readRelease(rwFlags);

	if (id > maxIndex || maxIndex == HWIDLIST_INDEX_INVALID)
	{
		/* Allocate new array, copy old one, free old one.
		 * Make sure to update arr.rsrc.maxIndex.
		 **/
		tmp = new arrayNodeS[id+1];
		if (tmp == __KNULL) {
			return ERROR_MEMORY_NOMEM;
		};

		arr.lock.readAcquire(&rwFlags);
		memcpy(tmp, arr.rsrc.arr,
			sizeof(arrayNodeS)
				* ((maxIndex == HWIDLIST_INDEX_INVALID)
					? 1
					: maxIndex + 1));

		arr.lock.readReleaseWriteAcquire(rwFlags);

		old = arr.rsrc.arr;
		arr.rsrc.arr = tmp;
		arr.rsrc.maxIndex = id;

		arr.lock.writeRelease();
		delete old;
	};

	// At this point there is enough space to hold the new item.
	arr.lock.writeAcquire();

	for (sarch_t i=arr.rsrc.firstValidIndex;
		i != HWIDLIST_INDEX_INVALID;)
	{
		if ((arr.rsrc.arr[i].next >= id)
			|| (arr.rsrc.arr[i].next == HWIDLIST_INDEX_INVALID))
		{
			itemNextIndex = arr.rsrc.arr[i].next;
			arr.rsrc.arr[i].next = id;
			break;
		}
		else {
			// Skip to the next valid index.
			i = arr.rsrc.arr[i].next;
		};
	};

	arr.rsrc.arr[id].item = item;
	arr.rsrc.arr[id].next = itemNextIndex;
	// Extra measure to ensure coherency across calls to the loop logic.
	__KFLAG_SET(arr.rsrc.arr[id].flags, HWIDLIST_FLAGS_INDEX_VALID);
	if (arr.rsrc.firstValidIndex == HWIDLIST_INDEX_INVALID) {
		arr.rsrc.firstValidIndex = id;
	};

	arr.lock.writeRelease();
	return ERROR_SUCCESS;
}

template <class T>
void hardwareIdListC<T>::removeItem(sarch_t id)
{
	uarch_t		rwFlags;
	sarch_t		maxIndex;

	arr.lock.readAcquire(&rwFlags);
	maxIndex = arr.rsrc.maxIndex;
	arr.lock.readRelease(rwFlags);

	if (id > maxIndex) {
		return;
	};

	arr.rsrc.writeAcquire();

	// If removing first item:
	if (arr.rsrc.firstValidIndex == id)
	{
		arr.rsrc.firstValidIndex = arr.rsrc.arr[id].next;
		__KFLAG_UNSET(
			arr.rsrc.arr[id].flags, HWIDLIST_FLAGS_INDEX_VALID);

		arr.lock.writeRelease();
		return;
	};

	for (sarch_t i=arr.rsrc.firstValidIndex; i != HWIDLIST_INDEX_INVALID; )
	{
		if (arr.rsrc.arr[i].next == id)
		{
			arr.rsrc.arr[i].next = arr.rsrc.arr[id].next;
			__KFLAG_UNSET(
				arr.rsrc.arr[id].flags,
				HWIDLIST_FLAGS_INDEX_VALID);

			arr.lock.writeRelease();
			return;
		};
		i = arr.rsrc.arr[i].next;
	};

	arr.lock.writeRelease();
}

#endif

