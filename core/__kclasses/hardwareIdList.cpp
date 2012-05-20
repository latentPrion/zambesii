
#include <debug.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <__kclasses/hardwareIdList.h>
#include <kernel/common/memoryTrib/memoryTrib.h>


hardwareIdListC::hardwareIdListC(void)
{
	arr.rsrc.maxIndex = HWIDLIST_INDEX_INVALID;
	arr.rsrc.firstValidIndex = HWIDLIST_INDEX_INVALID;
	arr.rsrc.arr = __KNULL;
}

void hardwareIdListC::dump(void)
{
	uarch_t		rwFlags;

	arr.lock.readAcquire(&rwFlags);

	__kprintf(NOTICE"HWID list @ 0x%p, arr @ v0x%p, dumping.\n",
		this, arr.rsrc.arr);

	for (sarch_t i=arr.rsrc.firstValidIndex; i != HWIDLIST_INDEX_INVALID;
		i=arr.rsrc.arr[i].next)
	{
		__kprintf(NOTICE HWIDLIST"%d (idx@0x%p): item 0x%p, next %d.\n",
			i, &arr.rsrc.arr[i].item, arr.rsrc.arr[i].item,
			arr.rsrc.arr[i].next);
	};

	arr.lock.readRelease(rwFlags);
}

void hardwareIdListC::__kspaceSetState(sarch_t id, void *arrayMem)
{
	arr.rsrc.arr = static_cast<arrayNodeS *>( arrayMem );
	arr.rsrc.maxIndex = id;
	arr.rsrc.firstValidIndex = id;
	arr.rsrc.arr[id].next = HWIDLIST_INDEX_INVALID;
}	

void *hardwareIdListC::getItem(sarch_t id)
{
	uarch_t		rwFlags;
	void		*ret=__KNULL;

	arr.lock.readAcquire(&rwFlags);

	if (id > arr.rsrc.maxIndex || id < arr.rsrc.firstValidIndex)
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

error_t hardwareIdListC::findFreeIndex(uarch_t *id)
{
	uarch_t		rwFlags;

	arr.lock.readAcquire(&rwFlags);

	for (uarch_t i=0; i<static_cast<uarch_t>( arr.rsrc.maxIndex + 1 ); i++)
	{
		if (!__KFLAG_TEST(
			arr.rsrc.arr[i].flags, HWIDLIST_FLAGS_INDEX_VALID))
		{
			arr.lock.readRelease(rwFlags);
			*id = i;
			return ERROR_SUCCESS;
		};
	};

	arr.lock.readRelease(rwFlags);
	return ERROR_GENERAL;
}

sarch_t hardwareIdListC::prepareForLoop(void)
{
	uarch_t		rwFlags;
	sarch_t		ret;

	arr.lock.readAcquire(&rwFlags);
	ret = arr.rsrc.firstValidIndex;
	arr.lock.readRelease(rwFlags);

	return ret;
}

void *hardwareIdListC::getLoopItem(sarch_t *context)
{
	uarch_t		rwFlags;
	void		*ret;

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

error_t hardwareIdListC::addItem(sarch_t index, void *item)
{
	uarch_t		rwFlags;
	sarch_t		maxIndex;
	arrayNodeS	*tmp, *old;

	arr.lock.readAcquire(&rwFlags);
	maxIndex = arr.rsrc.maxIndex;
	arr.lock.readRelease(rwFlags);

	// If the array must be resized, alloc new mem and copy over.
	if (index > maxIndex || maxIndex == HWIDLIST_INDEX_INVALID)
	{
		tmp = new (
			(memoryTrib.__kmemoryStream
				.*memoryTrib.__kmemoryStream.memAlloc)(
					PAGING_BYTES_TO_PAGES(
						sizeof(arrayNodeS)
							* (index + 1)),
					MEMALLOC_NO_FAKEMAP))
			arrayNodeS;

		if (tmp == __KNULL)
		{
			__kprintf(ERROR HWIDLIST"addItem(%d,0x%p): failed to "
				"resize array.\n",
				index, item);

			return ERROR_MEMORY_NOMEM;
		};

		arr.lock.writeAcquire();

		// Copy old array contents into new array.
		if (maxIndex != HWIDLIST_INDEX_INVALID)
		{
			memcpy(
				tmp, arr.rsrc.arr,
				sizeof(arrayNodeS) * (maxIndex + 1));
		};

		old = arr.rsrc.arr;
		arr.rsrc.arr = tmp;

		// Now add the new item to the array.
		if (maxIndex == HWIDLIST_INDEX_INVALID) {
			arr.rsrc.firstValidIndex = index;
		}
		else {
			arr.rsrc.arr[maxIndex].next = index;
		};

		arr.rsrc.maxIndex = index;
		arr.rsrc.arr[index].next = HWIDLIST_INDEX_INVALID;
		arr.rsrc.arr[index].item = item;
		__KFLAG_SET(
			arr.rsrc.arr[index].flags,
			HWIDLIST_FLAGS_INDEX_VALID);

		arr.lock.writeRelease();

		// Free the old array mem.
		if (old != __KNULL
			&& (!(reinterpret_cast<uarch_t>( old )
				& PAGING_BASE_MASK_LOW)))
		{
			memoryTrib.__kmemoryStream.memFree(old);
		};

		return ERROR_SUCCESS;
	};

	/* Non-resizeable cases: adding to the middle or front of a populated
	 * array.
	 **/
	arr.lock.writeAcquire();
	if (index < arr.rsrc.firstValidIndex)
	{
		arr.rsrc.arr[index].next = arr.rsrc.firstValidIndex;
		arr.rsrc.firstValidIndex = index;
	}
	else
	{
		for (sarch_t i=arr.rsrc.firstValidIndex;
			i != HWIDLIST_INDEX_INVALID;
			i = arr.rsrc.arr[i].next)
		{
			if (arr.rsrc.arr[i].next > index)
			{
				arr.rsrc.arr[index].next = arr.rsrc.arr[i].next;
				arr.rsrc.arr[i].next = index;
				break;
			};
		};
	};

	arr.rsrc.arr[index].item = item;
	__KFLAG_SET(arr.rsrc.arr[index].flags, HWIDLIST_FLAGS_INDEX_VALID);

	arr.lock.writeRelease();
	return ERROR_SUCCESS;
}

void hardwareIdListC::removeItem(sarch_t id)
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

	for (sarch_t i=arr.rsrc.firstValidIndex;
		i != HWIDLIST_INDEX_INVALID;
		i = arr.rsrc.arr[i].next)
	{
		if (arr.rsrc.arr[i].next == id)
		{
			if (id == arr.rsrc.maxIndex) {
				arr.rsrc.maxIndex = i;
			};

			arr.rsrc.arr[i].next = arr.rsrc.arr[id].next;
			__KFLAG_UNSET(
				arr.rsrc.arr[id].flags,
				HWIDLIST_FLAGS_INDEX_VALID);

			arr.lock.writeRelease();
			return;
		};
	};

	arr.lock.writeRelease();
}

