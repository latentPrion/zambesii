
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <__kclasses/hardwareIdList.h>
#include <kernel/common/processTrib/processTrib.h>


HardwareIdList::HardwareIdList(void)
{
	preAllocated = 0;
	arr.rsrc.maxIndex = HWIDLIST_INDEX_INVALID;
	arr.rsrc.maxAllocatedIndex = HWIDLIST_INDEX_INVALID;
	arr.rsrc.firstValidIndex = HWIDLIST_INDEX_INVALID;
	arr.rsrc.arr = NULL;
}

error_t HardwareIdList::initialize(
	void *preallocatedMem, ubit16 preallocatedSize
	)
{
	if (preallocatedMem == NULL) { return ERROR_SUCCESS; };

	preAllocated = 1;
	arr.rsrc.maxAllocatedIndex = (preallocatedSize / sizeof(sArrayNode) > 0)
		? preallocatedSize / sizeof(sArrayNode) - 1
		: arr.rsrc.maxAllocatedIndex;

	arr.rsrc.arr = reinterpret_cast<sArrayNode *>( preallocatedMem );
	memset(arr.rsrc.arr, 0, preallocatedSize);
	return ERROR_SUCCESS;
}

void HardwareIdList::dump(void)
{
	uarch_t		rwFlags;
	Iterator	it;

	arr.lock.readAcquire(&rwFlags);

	printf(NOTICE"HWID list @ %p, arr @ v%p, preAlloc'd: %d,\n"
		"\tmaxAllocatedIndex %d, firstValidIndex %d, maxIndex %d. "
		"dumping.\n",
		this, arr.rsrc.arr, preAllocated,
		arr.rsrc.maxAllocatedIndex, arr.rsrc.firstValidIndex,
		arr.rsrc.maxIndex);

	for (it = begin(); it != end(); ++it)
	{
		printf(NOTICE HWIDLIST"%d (idx@%p): item %p, next %d.\n",
			it.cursor, &arr.rsrc.arr[it.cursor].item,
			*it, arr.rsrc.arr[it.cursor].next);
	};

	arr.lock.readRelease(rwFlags);
}

void *HardwareIdList::getItem(sarch_t id)
{
	uarch_t		rwFlags;
	void		*ret=NULL;

	arr.lock.readAcquire(&rwFlags);

	if (id > arr.rsrc.maxIndex || id < arr.rsrc.firstValidIndex)
	{
		arr.lock.readRelease(rwFlags);
		return NULL;
	};

	if (FLAG_TEST(arr.rsrc.arr[id].flags, HWIDLIST_FLAGS_INDEX_VALID)) {
		ret = arr.rsrc.arr[id].item;
	};

	arr.lock.readRelease(rwFlags);
	return ret;
}

error_t HardwareIdList::findFreeIndex(uarch_t *id)
{
	uarch_t		rwFlags;

	arr.lock.readAcquire(&rwFlags);

	for (uarch_t i=0;
		i<static_cast<uarch_t>( arr.rsrc.maxAllocatedIndex + 1 );
		i++)
	{
		if (!FLAG_TEST(
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

void *HardwareIdList::getNextItem(sarch_t *cursor)
{
	uarch_t		rwFlags;
	void		*ret;

	arr.lock.readAcquire(&rwFlags);

	if (*cursor == HWIDLIST_INDEX_INVALID)
	{
		if (arr.rsrc.firstValidIndex == HWIDLIST_INDEX_INVALID) {
			ret = NULL;
		}
		else
		{
			*cursor = arr.rsrc.firstValidIndex;
			ret = arr.rsrc.arr[*cursor].item;
		};

		arr.lock.readRelease(rwFlags);
		return ret;
	};

	if (arr.rsrc.arr[*cursor].next == HWIDLIST_INDEX_INVALID) {
		ret = NULL;
	}
	else
	{
		*cursor = arr.rsrc.arr[*cursor].next;
		ret = arr.rsrc.arr[*cursor].item;
	};

	arr.lock.readRelease(rwFlags);
	return ret;
}

error_t HardwareIdList::addItem(sarch_t index, void *item)
{
	uarch_t		rwFlags;
	sarch_t		maxIndex, maxAllocatedIndex;
	sArrayNode	*tmp, *old;

	arr.lock.readAcquire(&rwFlags);
	maxIndex = arr.rsrc.maxIndex;
	maxAllocatedIndex = arr.rsrc.maxAllocatedIndex;
	arr.lock.readRelease(rwFlags);

	// If the array must be resized, alloc new mem and copy over.
	if (index > maxAllocatedIndex
		|| maxAllocatedIndex == HWIDLIST_INDEX_INVALID)
	{
		tmp = new (processTrib.__kgetStream()->memoryStream.memAlloc(
			PAGING_BYTES_TO_PAGES(
				sizeof(sArrayNode)
					* (index + 1)),
			MEMALLOC_NO_FAKEMAP))
				sArrayNode;

		if (tmp == NULL)
		{
			printf(ERROR HWIDLIST"addItem(%d,%p): failed to "
				"resize array.\n",
				index, item);

			return ERROR_MEMORY_NOMEM;
		};

		arr.lock.writeAcquire();

		// Copy old array contents into new array.
		if (maxAllocatedIndex != HWIDLIST_INDEX_INVALID)
		{
			memcpy(
				tmp, arr.rsrc.arr,
				sizeof(sArrayNode) * (maxAllocatedIndex + 1));
		}
		else
		{
			memset(
				tmp, 0,
				sizeof(sArrayNode) * (maxAllocatedIndex + 1));
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

		arr.rsrc.maxIndex = arr.rsrc.maxAllocatedIndex = index;
		arr.rsrc.arr[index].next = HWIDLIST_INDEX_INVALID;
		arr.rsrc.arr[index].item = item;
		FLAG_SET(
			arr.rsrc.arr[index].flags,
			HWIDLIST_FLAGS_INDEX_VALID);

		arr.lock.writeRelease();

		// Free the old array mem.
		if (!preAllocated && old != NULL) {
			processTrib.__kgetStream()->memoryStream.memFree(old);
		};

		return ERROR_SUCCESS;
	};

	/* Non-resizeable cases: adding to the middle or front of a populated
	 * array.
	 **/
	arr.lock.writeAcquire();
	if (arr.rsrc.firstValidIndex == HWIDLIST_INDEX_INVALID)
	{
		arr.rsrc.firstValidIndex = arr.rsrc.maxIndex = index;
		arr.rsrc.arr[index].next = HWIDLIST_INDEX_INVALID;
	}
	else if (index < arr.rsrc.firstValidIndex)
	{
		arr.rsrc.arr[index].next = arr.rsrc.firstValidIndex;
		arr.rsrc.firstValidIndex = index;
	}
	else if (index > maxIndex)
	{
		arr.rsrc.arr[maxIndex].next = index;
		arr.rsrc.arr[index].next = HWIDLIST_INDEX_INVALID;
		arr.rsrc.maxIndex = index;
	}
	else
	{
		for (sarch_t i=arr.rsrc.firstValidIndex;
			i != HWIDLIST_INDEX_INVALID;
			i = arr.rsrc.arr[i].next)
		{
			if (arr.rsrc.arr[i].next > index)
			{
				// If overwriting occupied slot, do nothing.
				if (i == index) { break; };

				arr.rsrc.arr[index].next = arr.rsrc.arr[i].next;
				arr.rsrc.arr[i].next = index;
				break;
			};
		};
	};

	arr.rsrc.arr[index].item = item;
	FLAG_SET(arr.rsrc.arr[index].flags, HWIDLIST_FLAGS_INDEX_VALID);

	arr.lock.writeRelease();
	return ERROR_SUCCESS;
}

void HardwareIdList::removeItem(sarch_t id)
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
		// If is also last item, don't forget to update maxIndex.
		if (id == arr.rsrc.maxIndex) {
			arr.rsrc.maxIndex = HWIDLIST_INDEX_INVALID;
		};

		FLAG_UNSET(
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
			FLAG_UNSET(
				arr.rsrc.arr[id].flags,
				HWIDLIST_FLAGS_INDEX_VALID);

			arr.lock.writeRelease();
			return;
		};
	};

	arr.lock.writeRelease();
}

