/*
 * File: udi_util.h
 *
 * Public definitions for UDI Utility functions.
 *
 * UDI drivers must not directly #include this file. It is included as
 * part of udi.h.
 *
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

#ifndef _UDI_CORE_UTIL_H
#define _UDI_CORE_UTIL_H

#ifndef _UDI_H_INSIDE
#error "udi_util.h must not be #included directly; use udi.h instead."
#endif

/*
 * String/memory utility functions.
 */

char *udi_strcat(
		char *s1,
		const char *s2);

char *udi_strncat(
		char *s1,
		const char *s2,
		udi_size_t n);

udi_sbit8_t udi_strcmp(
		const char *s1,
		const char *s2);

udi_sbit8_t udi_strncmp(
		const char *s1,
		const char *s2,
		udi_size_t n);

udi_sbit8_t udi_memcmp(
		const void *s1,
		const void *s2,
		udi_size_t n);

char *udi_strcpy(
		char *s1,
		const char *s2);

char *udi_strncpy(
		char *s1,
		const char *s2,
		udi_size_t n);

char *udi_strncpy_rtrim(
		char *s1,
		const char *s2,
		udi_size_t n);

void *udi_memcpy(
		void *s1,
		const void *s2,
		udi_size_t n);

void *udi_memmove(
		void *s1,
		const void *s2,
		udi_size_t n);

char *udi_strchr(
		const char *s,
		char c);

char *udi_strrchr(
		const char *s,
		char c);

void *udi_memchr(
		const void *s,
		udi_ubit8_t c,
		udi_size_t n);

udi_ubit32_t udi_strtou32(
		const char *s,
		char **endptr,
		int base);

udi_size_t udi_strlen(
		const char *s);

void *udi_memset(
		void *s,
		udi_ubit8_t c,
		udi_size_t n);

udi_size_t udi_snprintf(
		char *s,
		udi_size_t max_bytes,
		const char *format,
		...);

udi_size_t udi_vsnprintf(
		char *s,
		udi_size_t max_bytes,
		const char *format,
		va_list ap);

/* Note: This is not yet in the spec. */
void udi_debug_printf(const char *format,
		...);

/*
 * Queue Management utility functions.
 */

typedef struct udi_queue {
	struct udi_queue *next;
	struct udi_queue *prev;
} udi_queue_t;

void udi_enqueue(
	udi_queue_t *new_el,
	udi_queue_t *old_el);

udi_queue_t *udi_dequeue(
	udi_queue_t *element);

#define UDI_QUEUE_INIT(listhead) \
		((listhead)->next = (listhead)->prev = (listhead))
#define UDI_QUEUE_EMPTY(listhead) \
		((listhead)->next == (listhead))
#define UDI_ENQUEUE_HEAD(listhead, element) \
		udi_enqueue(element, listhead)
#define UDI_ENQUEUE_TAIL(listhead, element) \
		udi_enqueue(element, (listhead)->prev)
#define UDI_QUEUE_INSERT_AFTER(old_el, new_el) \
		udi_enqueue(new_el, old_el)
#define UDI_QUEUE_INSERT_BEFORE(old_el, new_el) \
		udi_enqueue(new_el, (old_el)->prev)
#define UDI_DEQUEUE_HEAD(listhead) \
		udi_dequeue((listhead)->next)
#define UDI_DEQUEUE_TAIL(listhead) \
		udi_dequeue((listhead)->prev)
#define UDI_QUEUE_REMOVE(element) \
		(void) udi_dequeue(element)
#define UDI_FIRST_ELEMENT(listhead) \
		((listhead)->next)
#define UDI_LAST_ELEMENT(listhead) \
		((listhead)->prev)
#define UDI_NEXT_ELEMENT(element) \
		((element)->next)
#define UDI_PREV_ELEMENT(element) \
		((element)->prev)
#define UDI_QUEUE_FOREACH(listhead, element, tmp) \
	    for ((element) = UDI_FIRST_ELEMENT(listhead); \
			(tmp) = UDI_NEXT_ELEMENT(element), \
			(element) != (listhead); \
			(element) = (tmp))

#define UDI_BASE_STRUCT(memberp, struct_type, member_name) \
		((struct_type *)((char *)(memberp) - \
				   offsetof(struct_type, member_name)))

/*
 * Endian Management utility functions.
 */

#define UDI_ENDIAN_SWAP_16(data16) \
		( (((data16) & 0x00ff) << 8) | \
		  (((data16) >> 8) & 0x00ff) )
#define UDI_ENDIAN_SWAP_32(data32) \
		( (((data32) & 0x000000ff) << 24) | \
		  (((data32) & 0x0000ff00) << 8)  | \
		  (((data32) >> 8) & 0x0000ff00)  | \
		  (((data32) >> 24) & 0x000000ff) )

/* Create a p, len bit mask (all ones in the bitfield, zeros elsewhere. */
#define UDI_BFMASK(p, len) \
		(((1U<<(len))-1) << (p))

/* Load unsigned p,len bitfield from val. */
#define UDI_BFGET(val, p, len) \
		(((udi_ubit8_t)(val) >> (p)) & ((1U<<(len))-1))

/* Store val into the p,len bitfield in dst. */
#define UDI_BFSET(val, p, len, dst) \
		((dst) = ((dst) & ~UDI_BFMASK(p, len)) | \
			(((udi_ubit8_t)(val) << (p)) & UDI_BFMASK(p, len)))

/* Extract a multi-byte value from a structure. */
#define UDI_MBGET(N, structp, field) \
	UDI_MBGET_##N (structp, field)
#define UDI_MBGET_2(structp, field) \
	((structp)->field##0 | ((structp)->field##1 << 8))
#define UDI_MBGET_3(structp, field) \
	((structp)->field##0 | ((structp)->field##1 << 8) | \
	((structp)->field##2 << 16))
#define UDI_MBGET_4(structp, field) \
	((structp)->field##0 | ((structp)->field##1 << 8) | \
	((structp)->field##2 << 16) | ((structp)->field##3 << 24))

/* Deposit "val" into a multi-byte field in a structure. */
#define UDI_MBSET(N, structp, field, val) \
	UDI_MBSET_##N (structp, field, val)
#define UDI_MBSET_2(structp, field, val) \
	((structp)->field##0 = (val) & 0xff, \
	(structp)->field##1 = ((val) >> 8) & 0xff)
#define UDI_MBSET_3(structp, field, val) \
	((structp)->field##0 = (val) & 0xff, \
	(structp)->field##1 = ((val) >> 8) & 0xff, \
	(structp)->field##2 = ((val) >> 16) & 0xff)
#define UDI_MBSET_4(structp, field, val) \
	((structp)->field##0 = (val) & 0xff, \
	(structp)->field##1 = ((val) >> 8) & 0xff, \
	(structp)->field##2 = ((val) >> 16) & 0xff, \
	(structp)->field##3 = ((val) >> 24) & 0xff)

void udi_endian_swap(
	const void *src,
	void *dst,
	udi_ubit8_t swap_size,
	udi_ubit8_t stride,
	udi_ubit16_t rep_count);

#define UDI_ENDIAN_SWAP_ARRAY(src, element_size, count) \
		udi_endian_swap(src, src, element_size, element_size, count)

#endif /* _UDI_CORE_UTIL_H */

