
#include <debug.h>
#define UDI_VERSION		0x101
#include <udi.h>
#define UDI_PHYSIO_VERSION	0x101
#include <udi_physio.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>
#include <kernel/common/floodplainn/channel.h>
#include <kernel/common/floodplainn/movableMemory.h>


struct sLayoutElementDescriptor
{
	uarch_t		elementSize,
			layoutSkipCount;
};

sLayoutElementDescriptor layoutElements[] =
{
	{ 0, 0 },				// UDI_DL_END

	// #def = 1, index = 1 (Fixed width types)
	#define ZUDI_LAYOUT_ELEMDESC_INTEGRAL_BASE	(1)
	{ sizeof(udi_ubit8_t), 1 },		// UDI_DL_UBIT8_T
	{ sizeof(udi_sbit8_t), 1 },		// UDI_DL_SBIT8_T
	{ sizeof(udi_ubit16_t), 1 },		// UDI_DL_UBIT16_T
	{ sizeof(udi_sbit16_t), 1 },		// UDI_DL_SBIT16_T
	{ sizeof(udi_ubit32_t), 1 },		// UDI_DL_UBIT32_T
	{ sizeof(udi_sbit32_t), 1 },		// UDI_DL_SBIT32_T
	{ sizeof(udi_boolean_t), 1 },		// UDI_DL_BOOLEAN_T
	{ sizeof(udi_status_t), 1 },		// UDI_DL_STATUS_T

	// #def = 20, index = 9 (Abstract types)
	#define ZUDI_LAYOUT_ELEMDESC_ABSTRACT_BASE	(9)
	{ sizeof(udi_index_t), 1 },		// UDI_DL_INDEX_T

	// #def = 30, index = 10 (Opaque handles)
	#define ZUDI_LAYOUT_ELEMDESC_OPAQUE_BASE	(10)
	{ sizeof(udi_channel_t), 1 },		// UDI_DL_CHANNEL_T
	{ 0, 1 },				// INVALID
	{ sizeof(udi_origin_t), 1 },		// UDI_DL_ORIGIN_T

	// #def = 40, index = 13 (Indirect element descriptors)
	#define ZUDI_LAYOUT_ELEMDESC_INDIRECT_BASE	(13)
	{ sizeof(udi_buf_t *), 4 },		// UDI_DL_BUF
	{ sizeof(udi_cb_t *), 1 },		// UDI_DL_CB
	{ sizeof(void *), 1 },			// UDI_DL_INLINE_UNTYPED
	{ sizeof(void *), 1 },			// UDI_DL_INLINE_DRIVER_TYPED
	{ sizeof(void *), 1 },			// UDI_DL_MOVABLE_UNTYPED

	// #def = 50, index = 18 (Nested element descriptors)
	#define ZUDI_LAYOUT_ELEMDESC_NESTED_BASE	(18)
	{ sizeof(void *), 0 },			// UDI_DL_INLINE_TYPED
	{ sizeof(void *), 0 },			// UDI_DL_MOVABLE_TYPED
	{ 0, 0 },				// UDI_DL_ARRAY

	// #def = 200, index = 21 (Physio opaque handles)
	#define ZUDI_LAYOUT_ELEMDESC_PHYSIO_BASE	(21)
	{ sizeof(udi_pio_handle_t), 1 },	// UDI_DL_PIO_HANDLE_T
	{ sizeof(udi_dma_constraints_t), 1 }	// UDI_DL_DMA_CONSTRAINTS_T
};

udi_size_t fplainn::sChannelMsg::zudi_layout_get_element_size(
	const udi_layout_t element, const udi_layout_t nestedLayout[],
	udi_size_t *layoutSkipCount
	)
{
	udi_index_t		index=element;

	// Physio opaque handle.
	if (element >= 200)
	{
		index = index - 200 + ZUDI_LAYOUT_ELEMDESC_PHYSIO_BASE;
		goto out;
	};

	// Nested element.
	if (element >= 50)
	{
		if (nestedLayout == NULL) {
			*layoutSkipCount = 0; return 0;
		};

		index = index - 50 + ZUDI_LAYOUT_ELEMDESC_NESTED_BASE;

		if (element == UDI_DL_INLINE_TYPED
			|| element == UDI_DL_MOVABLE_TYPED)
		{
			/* For these, the next few bytes until the next
			 * UDI_DL_END describe the structure of the object
			 * pointed to by the pointer element. Skip these
			 * detailed descriptor bytes, and increase the caller's
			 * skip count by the number of detail bytes as well.
			 *
			 * Skip-count is 2 because we need to skip both the
			 * DL_INLINE/DL_MOVABLE byte itself, AND the DL_END byte
			 * that terminates it.
			 **/
			*layoutSkipCount = 2;
			for (
				udi_layout_t *curr=nestedLayout;
				*curr != UDI_DL_END;
				curr++)
				{ *layoutSkipCount += 1; };

			return layoutElements[index].elementSize;
		};

		if (element == UDI_DL_ARRAY)
		{
			udi_layout_t		*curr;
			udi_size_t		nElements;
			status_t		elemSize=0;

			/* Skip count starts at 3, because the caller will
			 * have to skip both the UDI_DL_ARRAY byte and the
			 * element_count byte that follows it, AND the
			 * UDI_DL_END that terminates the sequence.
			 **/
			*layoutSkipCount = 3;

			curr = nestedLayout;
			nElements = *curr;
			curr++;

			elemSize = zudi_layout_get_size(curr, 0);
			if (elemSize < 0) { *layoutSkipCount = 0; return 0; };

			// Now count the number of elements to skip.
			for (udi_layout_t *laytmp=curr;
				laytmp != NULL && *laytmp != UDI_DL_END;
				laytmp++)
				{ *layoutSkipCount += 1; };

			return elemSize * nElements;
		}
		else
		{
			// Should never reach here.
			*layoutSkipCount = 0;
			return 0;
		};
	};

	// Indirect element pointer.
	if (element >= 40)
	{
		index = index - 40 + ZUDI_LAYOUT_ELEMDESC_INDIRECT_BASE;
		goto out;
	};

	// Opaque handle.
	if (element >= 30)
	{
		index = index - 30 + ZUDI_LAYOUT_ELEMDESC_OPAQUE_BASE;
		goto out;
	};

	// Abstract type.
	if (element >= 20)
	{
		index = index - 20 + ZUDI_LAYOUT_ELEMDESC_ABSTRACT_BASE;
		goto out;
	}
	else // Integral type.
	{
		index = index - 1 + ZUDI_LAYOUT_ELEMDESC_INTEGRAL_BASE;
		goto out;
	};

out:
	*layoutSkipCount = layoutElements[index].layoutSkipCount;
	return layoutElements[index].elementSize;
}


#include <kernel/common/floodplainn/movableMemory.h>
extern udi_enumerate_cb_t		*gecb;
status_t fplainn::sChannelMsg::marshalStackArguments(
	ubit8 *dest, va_list args, udi_layout_t *layout
	)
{
	uarch_t				offset=0, skipCount;
	uarch_t				elemSize;


	for (
		udi_layout_t *curr=layout;
		curr != NULL && *curr != UDI_DL_END;
		curr += skipCount, offset += elemSize)
	{
		/* We don't actually try to care about the type or size of the
		 * argument itself by using va_arg. We are only using it to
		 * ensure that we get the correct stack pop width when taking
		 * the arguments off the stack.
		 **/
		elemSize = zudi_layout_get_element_size(
			*curr, curr + 1, &skipCount);

		if (skipCount == 0) { return ERROR_INVALID_FORMAT; };

		// We don't support huge objects on the stack. Period.
		if (elemSize > 16) { return ERROR_UNSUPPORTED; };

		offset = align(offset, elemSize);
		switch (elemSize)
		{
		case 1: *(ubit8 *)(dest + offset) = 0xff; if (oo==56) {
fplainn::sMovableMemory *smm = (fplainn::sMovableMemory *)((uintptr_t)((udi_enumerate_cb_t *)gecb)->attr_list - sizeof(*smm));
printf(ERROR"in channel::send(): %d; dest 0x%p, off 0x%p, gecb 0x%p\n", smm->objectNBytes, dest, offset, gecb);
};
break;
		case 2: *(ubit16 *)(dest + offset) = va_arg(args, int); break;
		case 4: *(ubit32 *)(dest + offset) = va_arg(args, int); break;
		case 8:
#if __WORDSIZE == 32
			*(ubit32 *)(dest + offset) = va_arg(args, int);
			*(ubit32 *)(dest + offset + 4) = va_arg(args, int);
#elif __WORDSIZE >= 64
			*(ubit64 *)(dest + offset) = va_arg(args, int);
#else
#error "Unable to generate argument 8Byte marshalling for your arch wordsize."
#endif
			break;

		case 16:
#if __WORDSIZE == 32
			*(ubit32 *)(dest + offset) = va_arg(args, int);
			*(ubit32 *)(dest + offset + 4) = va_arg(args, int);
			*(ubit32 *)(dest + offset + 8) = va_arg(args, int);
			*(ubit32 *)(dest + offset + 12) = va_arg(args, int);
#elif __WORDSIZE == 64
			*(ubit64 *)(dest + offset) = va_arg(args, int);
			*(ubit64 *)(dest + offset + 8) = va_arg(args, int);
#elif __WORDSIZE >= 128
			*(ubit128 *)(dest + offset) = va_arg(args, int);
#else
#error "Unable to generate argument 16Byte marshalling for your arch wordsize."
#endif
			break;

		default:
			return ERROR_UNSUPPORTED;
		};
	};

	return ERROR_SUCCESS;
}

status_t fplainn::sChannelMsg::getTotalMarshalSpaceInlineRequirements(
	udi_layout_t *layout, udi_layout_t *drvTypedLayout,
	udi_cb_t *_srcCb
	)
{
	status_t		ret=0;
	uarch_t			ptrOffset=0, skipCount, currElemSize;
	ubit8			*srcCb = (ubit8 *)(_srcCb + 1);

	for (udi_layout_t *curr=layout;
		curr != NULL && *curr != UDI_DL_END;
		curr += skipCount, ptrOffset += currElemSize)
	{
		status_t			currLayoutSize=0;
		fplainn::sMovableMemory		*mmh;

		currElemSize = zudi_layout_get_element_size(
			*curr, curr + 1, &skipCount);

		if (skipCount == 0) { return ERROR_INVALID_FORMAT; };
		ptrOffset = align(ptrOffset, currElemSize);

		switch (*curr)
		{
		case UDI_DL_INLINE_TYPED:
			currLayoutSize = zudi_layout_get_size(curr + 1, 0);
			break;

		case UDI_DL_MOVABLE_TYPED:
			currLayoutSize = zudi_layout_get_size(curr + 1, 0);
			currLayoutSize += sizeof(fplainn::sMovableMemory);
			break;

		case UDI_DL_INLINE_DRIVER_TYPED:
			if (drvTypedLayout == NULL)
				{ return ERROR_INVALID_ARG; };

			currLayoutSize = zudi_layout_get_size(drvTypedLayout, 0);
			break;

		case UDI_DL_MOVABLE_UNTYPED:
			mmh = *(fplainn::sMovableMemory **)(srcCb + ptrOffset);
			if (mmh != NULL)
			{
				mmh--;
				currLayoutSize = mmh->objectNBytes;
				currLayoutSize +=
					sizeof(fplainn::sMovableMemory);
			}
			else {
				currLayoutSize = 0;
			};
			break;

		default: continue; break;
		}

		if (currLayoutSize < 0) { return currLayoutSize; };
		if (*(void **)(srcCb + ptrOffset) != NULL)
		{
			/* Only add it to the inline total if it's non-NULL.
			 * We have nothing to copy if it's NULL, and therefore
			 * there is no need to allocate marshal space for it.
			 **/
			ret += currLayoutSize;
		};
	}

	return ret;
}

extern udi_enumerate_cb_t		*gecb;
status_t fplainn::sChannelMsg::marshalInlineObjects(
	ubit8 *dest, udi_cb_t *_destCb, udi_cb_t *_srcCb,
	udi_layout_t *layout, udi_layout_t *drvTypedLayout
	)
{
	uarch_t			ptrOffset=0, skipCount, currElemSize;
	ubit8			*srcCb = (ubit8 *)(_srcCb + 1),
				*destCb = (ubit8 *)(_destCb + 1 );

	for (udi_layout_t *curr=layout;
		curr != NULL && *curr != UDI_DL_END;
		curr += skipCount, ptrOffset += currElemSize)
	{
		status_t		currObjectSize;
		fplainn::sMovableMemory	*mmh;
		void			*object;

		currElemSize = zudi_layout_get_element_size(
			*curr, curr + 1, &skipCount);

		if (skipCount == 0) { return ERROR_INVALID_FORMAT; };
		ptrOffset = align(ptrOffset, currElemSize);

/*if (oo==56)
{ printf(NOTICE">>>>>>>>%s.\n", gecb->attr_list[0].attr_name);};*/
		switch (*curr)
		{
		case UDI_DL_INLINE_TYPED:
			currObjectSize = zudi_layout_get_size(curr + 1, 0);
			break;

		case UDI_DL_MOVABLE_TYPED:
			currObjectSize = zudi_layout_get_size(curr + 1, 0);
			currObjectSize += sizeof(fplainn::sMovableMemory);
			break;

		case UDI_DL_INLINE_DRIVER_TYPED:
			currObjectSize = zudi_layout_get_size(drvTypedLayout, 0);
			break;

		case UDI_DL_MOVABLE_UNTYPED:
			mmh = *(fplainn::sMovableMemory **)(srcCb + ptrOffset);
			if (mmh != NULL)
			{
				mmh--;
				currObjectSize = mmh->objectNBytes;
				currObjectSize +=
					sizeof(fplainn::sMovableMemory);
			}
			else {
				currObjectSize = 0;
			};
printf(NOTICE"mov unt size %d.\n", currObjectSize);
			break;

		default: continue; break;
		};

		object = *(fplainn::sMovableMemory **)(srcCb + ptrOffset);
		if (object != NULL)
		{
			if (*curr == UDI_DL_MOVABLE_UNTYPED
				|| *curr == UDI_DL_MOVABLE_TYPED)
			{
				object = (fplainn::sMovableMemory *)object - 1;
			};

			// Copy the object into the marshal space.
/*if (oo==56)
{ printf(NOTICE"%%%%%%%%%s.\n", gecb->attr_list[0].attr_name);
printf(NOTICE"%%%%%%%% gecb 0x%p, dest 0x%p, %d bytes copied, %d.\n", gecb, dest, currObjectSize, sizeof(*gecb));
};*/
			memcpy(dest, object, currObjectSize);
			// Set the pointer in the destination CB.
			if (*curr == UDI_DL_MOVABLE_UNTYPED
				|| *curr == UDI_DL_MOVABLE_TYPED)
			{
				*(void **)(destCb + ptrOffset) =
					(fplainn::sMovableMemory *)dest + 1;
			}
			else {
				*(void **)(destCb + ptrOffset) = dest;
			};
			// Increment the marshal space.
			dest += currObjectSize;
		}
		else
		{
			/* There is probably no need to do this since the NULL
			 * would have been copied when the caller copied the
			 * visible mem (outside of this function). But it
			 * doesn't hurt to be thorough.
			 **/
			*(void **)(destCb + ptrOffset) = NULL;
		};
	};

	return ERROR_SUCCESS;
}

status_t fplainn::sChannelMsg::getTotalCbInlineRequirements(
	udi_layout_t *layout, udi_layout_t *drvTypedLayout
	)
{
	status_t		ret=0;
	uarch_t			skipCount, currElemSize;

	(void)currElemSize;

	for (udi_layout_t *curr=layout;
		curr != NULL && *curr != UDI_DL_END;
		curr += skipCount)
	{
		status_t			currLayoutSize=0;

		currElemSize = zudi_layout_get_element_size(
			*curr, curr + 1, &skipCount);

		if (skipCount == 0) { return ERROR_INVALID_FORMAT; };

		switch (*curr)
		{
		case UDI_DL_INLINE_TYPED:
			currLayoutSize = zudi_layout_get_size(curr + 1, 0);
			break;

		case UDI_DL_INLINE_DRIVER_TYPED:
			if (drvTypedLayout == NULL)
				{ return ERROR_INVALID_ARG; };

			currLayoutSize = zudi_layout_get_size(drvTypedLayout, 0);
			break;

		default: continue; break;
		}

		if (currLayoutSize < 0) { return currLayoutSize; };
		ret += currLayoutSize;
	}

	return ret;
}

error_t fplainn::sChannelMsg::initializeCbInlinePointers(
	udi_cb_t *cb, ubit8 *inlineSpace,
	udi_layout_t *layout, udi_layout_t *drvTypedLayout
	)
{
	status_t		inlineSpaceOffset=0;
	uarch_t			ptrOffset=0, skipCount, currElemSize;
	ubit8			*cb8 = (ubit8 *)(cb + 1);

	for (udi_layout_t *curr=layout;
		curr != NULL && *curr != UDI_DL_END;
		curr += skipCount, ptrOffset += currElemSize)
	{
		status_t			currLayoutSize=0;

		currElemSize = zudi_layout_get_element_size(
			*curr, curr + 1, &skipCount);

		if (skipCount == 0) { return ERROR_INVALID_FORMAT; };
		ptrOffset = align(ptrOffset, currElemSize);

		switch (*curr)
		{
		case UDI_DL_INLINE_TYPED:
			currLayoutSize = zudi_layout_get_size(curr + 1, 0);
			break;

		case UDI_DL_INLINE_DRIVER_TYPED:
			if (drvTypedLayout == NULL)
				{ return ERROR_INVALID_ARG; };

			currLayoutSize = zudi_layout_get_size(drvTypedLayout, 0);
			break;

		default: continue; break;
		}

		if (currLayoutSize < 0) { return currLayoutSize; };

		*(void **)(cb8 + ptrOffset) = inlineSpace + inlineSpaceOffset;
		inlineSpaceOffset += currLayoutSize;
	}

	return ERROR_SUCCESS;
}

status_t fplainn::sChannelMsg::updateClonedCbInlinePointers(
	udi_cb_t *cb, udi_cb_t *marshalledCb,
	udi_layout_t *layout, udi_layout_t *marshalLayout
	)
{
	uarch_t			ptrOffset=0, skipCount, currElemSize;
	ubit8			*cb8 = (ubit8 *)(cb + 1),
				*mcb8 = (ubit8 *)(marshalledCb + 1);
	status_t		marshalLayoutSize;

	/**	EXPLANATION:
	 * Takes a cloned control block and updates its inline pointer members
	 * to ensure that they are valid.
	 *
	 *	SEQUENCE:
	 * Must go through the visible_layout, seek each inline type, and for
	 * each one, read the source-cb. If the source-cb's pointer is
	 * non-NULL, we need to update the pointer in the target CB.
	 *
	 * We can actually do this /without/ knowing anything about the
	 * individual inline pointer types: to know where a pointer should point
	 * we need only know the offset within the CB that it should point to.
	 *
	 * We don't need to fully re-calculate the offsets using the layouts;
	 * Rather, we can simply use subtraction to determine how many bytes
	 * are between the sourceCB's pointer and its object, and set that
	 * same offset in the cloned cb.
	 **/
	marshalLayoutSize = fplainn::sChannelMsg::zudi_layout_get_size(
		marshalLayout, 0);

	if (marshalLayoutSize < 0) { return marshalLayoutSize; };

	for (udi_layout_t *curr=layout;
		curr != NULL && *curr != UDI_DL_END;
		curr += skipCount, ptrOffset += currElemSize)
	{
		ubit8			*srcObject;
		ptrdiff_t		objectOffset;

		currElemSize = zudi_layout_get_element_size(
			*curr, curr + 1, &skipCount);

		if (skipCount == 0) { return ERROR_INVALID_FORMAT; };
		ptrOffset = align(ptrOffset, currElemSize);

		switch (*curr)
		{
		case UDI_DL_INLINE_TYPED:
		case UDI_DL_INLINE_DRIVER_TYPED:
		case UDI_DL_MOVABLE_TYPED:
		case UDI_DL_MOVABLE_UNTYPED:
			break;

		default: continue; break;
		};

		srcObject = *(ubit8 **)(mcb8 + ptrOffset);
		if (srcObject == NULL)
		{
			// If ptr is NULL in source, set NULL in clone.
			*(void **)(cb8 + ptrOffset) = NULL;
			continue;
		};

		objectOffset = srcObject - ((ubit8 *)marshalledCb);
		*(void **)(cb8 + ptrOffset) =
			((ubit8 *)cb) - marshalLayoutSize + objectOffset;
	};

	return ERROR_SUCCESS;
}
