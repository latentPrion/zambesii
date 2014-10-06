/*
 * Taken from the UDI Reference Implementation.
 * File: env/common/udi_layout.c
 */

/*
 * $Copyright udi_reference:
 *
 *
 *    Copyright (c) 1995-2001; Compaq Computer Corporation; Hewlett-Packard
 *    Company; Interphase Corporation; The Santa Cruz Operation, Inc;
 *    Software Technologies Group, Inc; and Sun Microsystems, Inc
 *    (collectively, the "Copyright Holders").  All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the conditions are met:
 *
 *            Redistributions of source code must retain the above
 *            copyright notice, this list of conditions and the following
 *            disclaimer.
 *
 *            Redistributions in binary form must reproduce the above
 *            copyright notice, this list of conditions and the following
 *            disclaimers in the documentation and/or other materials
 *            provided with the distribution.
 *
 *            Neither the name of Project UDI nor the names of its
 *            contributors may be used to endorse or promote products
 *            derived from this software without specific prior written
 *            permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *    "AS IS," AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *    HOLDERS OR ANY CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 *    OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 *    TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 *    USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 *    DAMAGE.
 *
 *    THIS SOFTWARE IS BASED ON SOURCE CODE PROVIDED AS A SAMPLE REFERENCE
 *    IMPLEMENTATION FOR VERSION 1.01 OF THE UDI CORE SPECIFICATION AND/OR
 *    RELATED UDI SPECIFICATIONS. USE OF THIS SOFTWARE DOES NOT IN AND OF
 *    ITSELF CONSTITUTE CONFORMANCE WITH THIS OR ANY OTHER VERSION OF ANY
 *    UDI SPECIFICATION.
 *
 *
 * $
 */

#define UDI_VERSION		0x101
#include <udi.h>
#define UDI_PHYSIO_VERSION	0x101
#include <udi_physio.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>
#include <commonlibs/libzbzcore/libzudi.h>


#define _UDI_PHYSIO_SUPPORTED		1

// FIXME: I do *not* like this macro at all.
#define _ported_udi_alignto(_item, boundary) \
  (((unsigned)((uintptr_t)(_item) + (boundary) - 1) >= (unsigned)((uintptr_t) (_item)))	\
   ? (((uintptr_t) (_item) + ((boundary) - 1)) & (~((uintptr_t)(boundary)-1)))	\
   : ~ (long) 0)

/*
 * Analyze a layout template and see how much core storage it will take.
 *
 * Has the additional benefit that it will validate the layout template
 * before we ever try to do anything "hard" with it.
 */
namespace lzudi
{

udi_size_t _udi_get_layout_size(
	udi_layout_t *layout,
	udi_ubit16_t *inline_offset,
	udi_ubit16_t *chain_offset
	)
{
	udi_size_t		layout_size=0, element_size, array_size=0;
	udi_boolean_t		found_dl_cb=FALSE;

	if (layout == NULL) { return 0;	};

	for (;;)
	{
		switch (*layout)
		{
		case UDI_DL_UBIT8_T:
			element_size = sizeof(udi_ubit8_t);
			break;

		case UDI_DL_SBIT8_T:
			element_size = sizeof(udi_sbit8_t);
			break;

		case UDI_DL_UBIT16_T:
			element_size = sizeof(udi_ubit16_t);
			break;

		case UDI_DL_SBIT16_T:
			element_size = sizeof(udi_sbit16_t);
			break;

		case UDI_DL_UBIT32_T:
			element_size = sizeof(udi_ubit32_t);
			break;

		case UDI_DL_SBIT32_T:
			element_size = sizeof(udi_sbit32_t);
			break;

		case UDI_DL_BOOLEAN_T:
			element_size = sizeof(udi_boolean_t);
			break;

		case UDI_DL_STATUS_T:
			element_size = sizeof(udi_status_t);
			break;

		case UDI_DL_INDEX_T:
			element_size = sizeof(udi_index_t);
			break;

		case UDI_DL_CHANNEL_T:
			element_size = sizeof(udi_channel_t);
			break;

		case UDI_DL_ORIGIN_T:
			element_size = sizeof(udi_origin_t);
			break;

#if _UDI_PHYSIO_SUPPORTED
		case UDI_DL_DMA_CONSTRAINTS_T:
			element_size = sizeof(udi_dma_constraints_t);
			break;
#endif /* _UDI_PHYSIO_SUPPORTED */

		case UDI_DL_BUF:
			element_size = sizeof(udi_buf_t *);
			layout += 3;	/* skip "uninteresting" args */
			break;

		case UDI_DL_CB:
			element_size = sizeof(udi_cb_t *);

			/*
			 * The spec says we can have at most one of these
			 * per cb.  Enforce that now.
			 */
			assert_fatal(found_dl_cb == FALSE);
			found_dl_cb = TRUE;

			/*
			 * Make the offset of CB pointers available
			 * to the caller.
			 */
			if (chain_offset)
			{
				*chain_offset = sizeof(udi_cb_t)
					+ _ported_udi_alignto(
						layout_size, element_size);
			}
			break;

		case UDI_DL_INLINE_UNTYPED:
		case UDI_DL_INLINE_DRIVER_TYPED:
			element_size = sizeof(void *);

			/*
			 * Make the offset of inline types available
			 * to the caller.
			 */
			if (inline_offset)
			{
				*inline_offset = sizeof(udi_cb_t)
					+ _ported_udi_alignto(
						layout_size, element_size);
			}
			break;

		case UDI_DL_INLINE_TYPED:
			/*
			 * The size in the surrounding object is
			 * just a pointer, but we have to skip over
			 * the nested layout.
			 * TODO: Something will probably care about
			 * the content of the nested layout.
			 */
			element_size = sizeof(void *);

			/*
			 * Make the offset of inline types available
			 * to the caller.
			 */
			if (inline_offset)
			{
				*inline_offset = sizeof(udi_cb_t)
					+ _ported_udi_alignto(
						layout_size, element_size);
			}

			/*
			 * This isn't very efficient, but we're in
			 * a low-running path and this keeps the
			 * calling convention simple.
			 */
			while (*++layout != UDI_DL_END);
			break;

		case UDI_DL_MOVABLE_UNTYPED:
			element_size = sizeof(void *);
			break;

		case UDI_DL_MOVABLE_TYPED:
			/*
			 * The size in the surrounding object is
			 * just a pointer, but we have to skip over
			 * the nested layout.
			 * TODO: Something will probably care about
			 * the content of the nested layout.
			 */
			element_size = sizeof(void *);

			/*
			 * This isn't very efficient, but we're in
			 * a low-running path and this keeps the
			 * calling convention simple.
			 */
			while (*++layout != UDI_DL_END);
			break;

		case UDI_DL_ARRAY:
			/*
			 * Recurse to parse these
			 */
			array_size = *++layout;
			element_size = array_size
				* _udi_get_layout_size(++layout, NULL, NULL);
			/*
			 * This isn't very efficient, but we're in
			 * a low-running path and this keeps the
			 * calling convention simple.
			 */
			while (*++layout != UDI_DL_END);
			break;

#if defined (UDI_DL_PIO_HANDLE_T)
		case UDI_DL_PIO_HANDLE_T:
			element_size = sizeof(udi_pio_handle_t);
			break;
#endif

		case UDI_DL_END:
			return layout_size;

		default:
printf(NOTICE"unknown UDI_DL %d.\n", *layout);
			assert_fatal(0);
		}
		layout++;

		/*
		 * Properly align this element
		 */
		layout_size = _ported_udi_alignto(layout_size, element_size);

		/*
		 * Add the element to the total size
		 */
		layout_size += element_size;
	}
}

#define STORE_ARG(ptr, ap, type, va) \
	element_size = sizeof(type); \
	ptr = (void *) (_ported_udi_alignto(ptr, element_size)); \
	*(type *) ptr = _##va(args, type);

void _udi_marshal_params(
	udi_layout_t *layout, void *marshal_space, va_list args
	)
{
	udi_size_t			element_size, array_size=0;
	void				*p=marshal_space;

	if (layout == NULL) { return; };

	for (;;)
	{
		switch (*layout)
		{
		case UDI_DL_UBIT8_T:
			STORE_ARG(p, args, udi_ubit8_t, UDI_VA_UBIT8_T);
			break;
		case UDI_DL_SBIT8_T:
			STORE_ARG(p, args, udi_sbit8_t, UDI_VA_SBIT8_T);
			break;
		case UDI_DL_UBIT16_T:
			STORE_ARG(p, args, udi_ubit16_t, UDI_VA_UBIT16_T);
			break;
		case UDI_DL_SBIT16_T:
			STORE_ARG(p, args, udi_sbit16_t, UDI_VA_SBIT16_T);
			break;
		case UDI_DL_UBIT32_T:
			STORE_ARG(p, args, udi_ubit32_t, UDI_VA_UBIT32_T);
			break;
		case UDI_DL_SBIT32_T:
			STORE_ARG(p, args, udi_sbit32_t, UDI_VA_SBIT32_T);
			break;
		case UDI_DL_BOOLEAN_T:
			STORE_ARG(p, args, udi_boolean_t, UDI_VA_BOOLEAN_T);
			break;
		case UDI_DL_STATUS_T:
			STORE_ARG(p, args, udi_status_t, UDI_VA_STATUS_T);
			break;

		case UDI_DL_INDEX_T:
			STORE_ARG(p, args, udi_index_t, UDI_VA_INDEX_T);
			break;

		case UDI_DL_CHANNEL_T:
			STORE_ARG(p, args, udi_channel_t, UDI_VA_CHANNEL_T);
			break;

		case UDI_DL_ORIGIN_T:
			STORE_ARG(p, args, udi_origin_t, UDI_VA_ORIGIN_T);
			break;

#if _UDI_PHYSIO_SUPPORTED
		case UDI_DL_DMA_CONSTRAINTS_T:
			STORE_ARG(
				p, args, udi_dma_constraints_t,
				UDI_VA_DMA_CONSTRAINTS_T);

			break;
#endif /* _UDI_PHYSIO_SUPPORTED */

		case UDI_DL_BUF:
			STORE_ARG(p, args, udi_buf_t *, UDI_VA_POINTER);
			layout += 3;	/* skip "uninteresting" args */
			break;
		case UDI_DL_CB:
			STORE_ARG(p, args, udi_cb_t *, UDI_VA_POINTER);
			break;

		case UDI_DL_INLINE_UNTYPED:
		case UDI_DL_INLINE_DRIVER_TYPED:
		case UDI_DL_MOVABLE_UNTYPED:
			STORE_ARG(p, args, void *, UDI_VA_POINTER);
			break;

		case UDI_DL_MOVABLE_TYPED:
		case UDI_DL_INLINE_TYPED:
			while (*++layout != UDI_DL_END);
			STORE_ARG(p, args, void *, UDI_VA_POINTER);
			break;

		case UDI_DL_ARRAY:
			/*
			 * Recurse to parse these
			 */
			array_size = *++layout;
			element_size = array_size
				* _udi_get_layout_size(++layout, NULL, NULL);

			/*
			 * This isn't very efficient, but we're in
			 * a low-running path and this keeps the
			 * calling convention simple.
			 */
			while (*++layout != UDI_DL_END);
			break;

		case UDI_DL_END:
			return;

		default:
			assert_fatal(0);
		}
		layout++;

		p = (char *)p + element_size;
	}
}


/*
 * look for the first occurance a layout_t and return the offset.
 * in the special case of UDI_DL_END, intermmediate nested
 * structures are ignored. ie:
 * UDI_UBIT8_T UDI_DL_ARRAY UDI_UBIT8_T 10 UDI_DL_END UDI_DL_END
 * _udi_get_layout_offset(layout, UDI_DL_END) would return
 * on the second UDI_DL_END
 */
udi_boolean_t _udi_get_layout_offset(
	udi_layout_t *start, udi_layout_t **end, udi_size_t *offset,
	udi_layout_t key
	)
{

	udi_layout_t			*layout=start;
	udi_size_t			layout_size=0, element_size,
					array_size=0;

	if (layout == NULL) { return FALSE; };

	while (*layout != key)
	{
		switch (*layout)
		{
		case UDI_DL_UBIT8_T:
			element_size = sizeof(udi_ubit8_t);
			break;
		case UDI_DL_SBIT8_T:
			element_size = sizeof(udi_sbit8_t);
			break;
		case UDI_DL_UBIT16_T:
			element_size = sizeof(udi_ubit16_t);
			break;
		case UDI_DL_SBIT16_T:
			element_size = sizeof(udi_sbit16_t);
			break;
		case UDI_DL_UBIT32_T:
			element_size = sizeof(udi_ubit32_t);
			break;
		case UDI_DL_SBIT32_T:
			element_size = sizeof(udi_sbit32_t);
			break;
		case UDI_DL_BOOLEAN_T:
			element_size = sizeof(udi_boolean_t);
			break;
		case UDI_DL_STATUS_T:
			element_size = sizeof(udi_status_t);
			break;

		case UDI_DL_INDEX_T:
			element_size = sizeof(udi_index_t);
			break;

		case UDI_DL_CHANNEL_T:
			element_size = sizeof(udi_channel_t);
			break;

		case UDI_DL_ORIGIN_T:
			element_size = sizeof(udi_origin_t);
			break;

#if _UDI_PHYSIO_SUPPORTED
		case UDI_DL_DMA_CONSTRAINTS_T:
			element_size = sizeof(udi_dma_constraints_t);
			break;
#endif /* _UDI_PHYSIO_SUPPORTED */

		case UDI_DL_BUF:
			element_size = sizeof(udi_buf_t *);
			layout += 3;	/* skip "uninteresting" args */
			break;

		case UDI_DL_CB:
			element_size = sizeof(udi_cb_t *);
			break;

		case UDI_DL_INLINE_UNTYPED:
		case UDI_DL_INLINE_DRIVER_TYPED:
			element_size = sizeof(void *);
			break;

		case UDI_DL_INLINE_TYPED:
			/*
			 * The size in the surrounding object is
			 * just a pointer, but we have to skip over
			 * the nested layout.
			 * TODO: Something will probably care about
			 * the content of the nested layout.
			 */
			element_size = sizeof(void *);
			while (*++layout != UDI_DL_END);
			break;

		case UDI_DL_MOVABLE_UNTYPED:
			element_size = sizeof(void *);
			break;

		case UDI_DL_MOVABLE_TYPED:
			/*
			 * The size in the surrounding object is
			 * just a pointer, but we have to skip over
			 * the nested layout.
			 * TODO: Something will probably care about
			 * the content of the nested layout.
			 */
			element_size = sizeof(void *);
			while (*++layout != UDI_DL_END);
			break;

		case UDI_DL_ARRAY:
			/*
			 * Recurse to parse these
			 */
			array_size = *++layout;
			element_size = array_size
				* _udi_get_layout_size(++layout, NULL, NULL);

			while (*++layout != UDI_DL_END);
			break;

#if defined (UDI_DL_PIO_HANDLE_T)
		case UDI_DL_PIO_HANDLE_T:
			element_size = sizeof(udi_pio_handle_t);
			break;
#endif

		case UDI_DL_END:
			return FALSE;
		default:
			assert_fatal(0);
		}
		layout++;

		/*
		 * Properly align this element
		 */
		layout_size = _ported_udi_alignto(layout_size, element_size);

		/*
		 * Add the element to the total size
		 */
		layout_size += element_size;
	}
	*end = layout;
	*offset = layout_size;
	return TRUE;
}

}
