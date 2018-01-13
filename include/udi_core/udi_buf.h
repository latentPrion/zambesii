/*
 * File: udi_buf.h
 *
 * Public definitions for UDI buffer management.
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

#ifndef _UDI_CORE_BUF_H
#define _UDI_CORE_BUF_H

#ifndef _UDI_H_INSIDE
#error "udi_buf.h must not be #included directly; use udi.h instead."
#endif


/*
 * Semi-opaque type.
 */
#define ZUDI_MAX_N_CONSTRAINTS_ATTRS	(32)
typedef struct __udi_buf {
	udi_size_t buf_size;
	/* Zambesii needs to have the constraints attributes associated with a
	 * buffer included in the userspace buffer handle, in order to enable
	 * the userspace UDI library to properly implement udi_buf_read() and
	 * udi_buf_write().
	 *
	 * If you associate a UDI buf with a buf_path, the Zambesii kernel will
	 * also associate that UDI buf with the constraints of the parent device
	 * which that buf_path points to.
	 *
	 * The reason for this is to enable zero-copy passthrough of buffers
	 * all the way from userspace to the end-device's DMA engine.
	 **/
	udi_dma_constraints_attr_spec_t constraints[ZUDI_MAX_N_CONSTRAINTS_ATTRS];
} udi_buf_t;

/*
 * Buffer Management Macros
 */

#define UDI_BUF_ALLOC( \
		callback, gcb, init_data, size, path_handle) \
			udi_buf_write(callback, gcb, init_data, \
				size, NULL, 0, 0, path_handle)

#define UDI_BUF_INSERT( \
		callback, gcb, new_data, size, dst_buf, dst_off) \
			udi_buf_write(callback, gcb, new_data, size, \
				dst_buf, dst_off, 0, UDI_NULL_BUF_PATH)

#define UDI_BUF_DELETE( \
		callback, gcb, size, dst_buf, dst_off) \
			udi_buf_write(callback, gcb, NULL, 0, \
				dst_buf, dst_off, size, \
				UDI_NULL_BUF_PATH)
#define UDI_BUF_DUP( \
		callback, gcb, src_buf, path_handle) \
			udi_buf_copy(callback, gcb, src_buf, \
			0, (src_buf)->buf_size, NULL, \
			0, 0, path_handle)

/*
 * Function prototypes for Buffer Management Service Calls
 */

typedef void	udi_buf_copy_call_t (
			udi_cb_t *gcb,
			udi_buf_t *new_dst_buf);

typedef void	udi_buf_write_call_t (
			udi_cb_t *gcb,
			udi_buf_t *new_dst_buf);

typedef void	udi_buf_path_alloc_call_t (
			udi_cb_t *gcb,
			udi_buf_path_t new_buf_path);

void udi_buf_path_alloc(
		udi_buf_path_alloc_call_t *callback,
		udi_cb_t *gcb);

void udi_buf_path_free(
		udi_buf_path_t buf_path_handle);

void udi_buf_copy(
		udi_buf_copy_call_t *callback,
		udi_cb_t *gcb,
		udi_buf_t *src_buf,
		udi_size_t src_off,
		udi_size_t src_len,
		udi_buf_t *dst_buf,
		udi_size_t dst_off,
		udi_size_t dst_len,
		udi_buf_path_t path_handle);

void udi_buf_read(
		udi_buf_t *src_buf,
		udi_size_t src_off,
		udi_size_t src_len,
		void *dst_mem);

void udi_buf_write(
		udi_buf_write_call_t *callback,
		udi_cb_t *gcb,
		const void *src_mem,
		udi_size_t src_len,
		udi_buf_t *dst_buf,
		udi_size_t dst_off,
		udi_size_t dst_len,
		udi_buf_path_t path_handle);

void udi_buf_free(
		udi_buf_t *buf);

void udi_buf_path_alloc(
		udi_buf_path_alloc_call_t *callback,
		udi_cb_t *gcb);

void udi_buf_best_path(
		udi_buf_t *buf,
		udi_buf_path_t *path_handles,
		udi_ubit8_t npaths,
		udi_ubit8_t last_fit,
		udi_ubit8_t *best_fit_array);

#define UDI_BUF_PATH_END	255
		

/*
 * Definitions for buffer tags.
 */

typedef udi_ubit32_t udi_tagtype_t;

/* Tag Category Masks */
#define UDI_BUFTAG_ALL 0xffffffff
#define UDI_BUFTAG_VALUES 0x000000ff
#define UDI_BUFTAG_UPDATES 0x0000ff00
#define UDI_BUFTAG_STATUS 0x00ff0000
#define UDI_BUFTAG_DRIVERS 0xff000000

/* Value Category Tag Types */
#define UDI_BUFTAG_BE16_CHECKSUM (1U<<0)

/* Update Category Tag Types */
#define UDI_BUFTAG_SET_iBE16_CHECKSUM (1U<<8)
#define UDI_BUFTAG_SET_TCP_CHECKSUM (1U<<9)
#define UDI_BUFTAG_SET_UDP_CHECKSUM (1U<<10)

/* Status Category Tag Types */
#define UDI_BUFTAG_TCP_CKSUM_GOOD (1U<<17)
#define UDI_BUFTAG_UDP_CKSUM_GOOD (1U<<18)
#define UDI_BUFTAG_IP_CKSUM_GOOD (1U<<19)
#define UDI_BUFTAG_TCP_CKSUM_BAD (1U<<21)
#define UDI_BUFTAG_UDP_CKSUM_BAD (1U<<22)
#define UDI_BUFTAG_IP_CKSUM_BAD (1U<<23)

/* Driver-internal Category Tag Types */
#define UDI_BUFTAG_DRIVER1 (1U<<24)
#define UDI_BUFTAG_DRIVER2 (1U<<25)
#define UDI_BUFTAG_DRIVER3 (1U<<26)
#define UDI_BUFTAG_DRIVER4 (1U<<27)
#define UDI_BUFTAG_DRIVER5 (1U<<28)
#define UDI_BUFTAG_DRIVER6 (1U<<29)
#define UDI_BUFTAG_DRIVER7 (1U<<30)
#define UDI_BUFTAG_DRIVER8 (1U<<31)

/* Buffer tag structure */
typedef struct _udi_buf_tag_t {
	udi_tagtype_t tag_type;
	udi_ubit32_t tag_value;
	udi_size_t tag_off;
	udi_size_t tag_len;
} udi_buf_tag_t;

typedef void udi_buf_tag_set_call_t(
	udi_cb_t *gcb,
	udi_buf_t *new_buf);

typedef void udi_buf_tag_apply_call_t(
	udi_cb_t *gcb,
	udi_buf_t *new_buf);

void udi_buf_tag_set(
	udi_buf_tag_set_call_t *callback,
	udi_cb_t *gcb,
	udi_buf_t *buf,
	udi_buf_tag_t *tag_array,
	udi_ubit16_t tag_array_length);

udi_ubit16_t udi_buf_tag_get(
	udi_buf_t *buf,
	udi_tagtype_t tag_type,
	udi_buf_tag_t *tag_array,
	udi_ubit16_t tag_array_length,
	udi_ubit16_t tag_start_idx);

udi_ubit32_t udi_buf_tag_compute(
	udi_buf_t *buf,
	udi_size_t off,
	udi_size_t len,
	udi_tagtype_t tag_type);

void udi_buf_tag_apply(
	udi_buf_tag_apply_call_t *callback,
	udi_cb_t *gcb,
	udi_buf_t *buf,
	udi_tagtype_t tag_type);

#endif /* _UDI_CORE_BUF_H */

