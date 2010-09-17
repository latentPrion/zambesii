#ifndef _HARDWARE_ID_LIST_H
	#define _HARDWARE_ID_LIST_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kflagManipulation.h>
	#include <__kstdlib/__kclib/string.h>
	#include <__kstdlib/__kcxxlib/new>
	#include <__kclasses/debugPipe.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/multipleReaderLock.h>
	#include <kernel/common/memoryTrib/memoryTrib.h>

#define HWIDLIST_INDEX_INVALID		(-1)

#define HWIDLIST_FLAGS_INDEX_VALID	(1<<0)

template <class T>
class hardwareIdListC
{
public:
	hardwareIdListC(void);
	void __kspaceSetState(sarch_t id, void *arrayMem);
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

public:
	struct arrayNodeS
	{
		sarch_t		next;
		uarch_t		flags;
		T		*item;
	};

private:
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
void hardwareIdListC<T>::__kspaceSetState(sarch_t id, void *arrayMem)
{
	arr.rsrc.arr = static_cast<arrayNodeS *>( arrayMem );
	arr.rsrc.maxIndex = id;
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

	if (*context < 0) {
		return __KNULL;
	};

	// 'context' is an index into the array.
	arr.lock.readAcquire(&rwFlags);

	if (!__KFLAG_TEST(
		arr.rsrc.arr[*context].flags, HWIDLIST_FLAGS_INDEX_VALID))
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
	sarch_t		maxIndex, itemNextIndex;
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
		tmp = new (
			(memoryTrib.__kmemoryStream
				.*memoryTrib.__kmemoryStream.memAlloc)(
					PAGING_BYTES_TO_PAGES(
						sizeof(arrayNodeS) * (id + 1)),
					MEMALLOC_NO_FAKEMAP))
			arrayNodeS;

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

		// Avoid freeing __kspace stream if that's what we're handling.
		if (!(reinterpret_cast<uarch_t>( old ) & PAGING_BASE_MASK_LOW))
		{
			memoryTrib.__kmemoryStream.memFree(old);
		};
	};

	// At this point there is enough space to hold the new item.
	arr.lock.writeAcquire();

	if (arr.rsrc.firstValidIndex == HWIDLIST_INDEX_INVALID)
	{
		itemNextIndex = HWIDLIST_INDEX_INVALID;
		arr.rsrc.firstValidIndex = id;
	}
	else if (id < arr.rsrc.firstValidIndex)
	{
		itemNextIndex = arr.rsrc.firstValidIndex;
		arr.rsrc.firstValidIndex = id;
	}
	else
	{
		for (sarch_t i=arr.rsrc.firstValidIndex;
			i != HWIDLIST_INDEX_INVALID;
			i = arr.rsrc.arr[i].next)
		{
			if (id < arr.rsrc.arr[i].next
				|| (arr.rsrc.arr[i].next
					== HWIDLIST_INDEX_INVALID))
			{
				itemNextIndex = arr.rsrc.arr[i].next;
				arr.rsrc.arr[i].next = id;
				break;
			};
		};
	};

	arr.rsrc.arr[id].item = item;
	arr.rsrc.arr[id].next = itemNextIndex;
	// Extra measure to ensure coherency across calls to the loop logic.
	__KFLAG_SET(arr.rsrc.arr[id].flags, HWIDLIST_FLAGS_INDEX_VALID);

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

	arr.lock.writeAcquire();

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

