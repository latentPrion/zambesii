/*
 * File: udi_cb.h
 *
 * Public definitions for Control Block Management.
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

#ifndef _UDI_CORE_CB_H
#define _UDI_CORE_CB_H

#ifndef _UDI_H_INSIDE
#error "udi_cb.h must not be #included directly; use udi.h instead."
#endif

/*
 * Opaque types.
 */
typedef _UDI_HANDLE udi_channel_t;
typedef _UDI_HANDLE udi_origin_t;

/*
 * A null value for origin handles.
 */
#define _UDI_NULL_ORIGIN ((udi_origin_t)NULL)

/*
 * Generic, least-common-denominator control block 
 */
typedef struct {
	udi_channel_t channel;
	void *context;
	void *scratch;
	void *initiator_context;
	udi_origin_t origin;
} udi_cb_t;

/*
 * Macros for Control Block mgmt
 */
#define	UDI_GCB(mcb)  (&(mcb)->gcb)
#define	UDI_MCB(gcb, cb_type)  ((cb_type *)(gcb))

/*
 * Callback types.
 */
typedef void udi_cb_alloc_call_t(
		udi_cb_t *gcb,
		udi_cb_t *new_cb);

typedef void udi_cb_alloc_batch_call_t(
		udi_cb_t *gcb,
		udi_cb_t *first_new_cb);

typedef	void udi_cancel_call_t(
		udi_cb_t *gcb);

/*
 * Function prototypes for control block mgmt services.
 */
void udi_cb_alloc(
		udi_cb_alloc_call_t *callback,
		udi_cb_t *gcb,
		udi_index_t cb_idx,
		udi_channel_t default_channel);

void udi_cb_alloc_dynamic(
		udi_cb_alloc_call_t *callback,
		udi_cb_t *gcb,
		udi_index_t cb_idx,
		udi_channel_t default_channel,
		udi_size_t inline_size,
		udi_layout_t *inline_layout);

void udi_cb_alloc_batch(
		udi_cb_alloc_batch_call_t *calback,
		udi_cb_t *gcb,
		udi_index_t cb_idx,
		udi_index_t count,
		udi_boolean_t with_buf,
		udi_size_t buf_size,
		udi_buf_path_t path_handle);

void udi_cb_free(
		udi_cb_t *cb);

void udi_cancel(
		udi_cancel_call_t *callback,
		udi_cb_t *gcb);

#endif /* _UDI_CORE_CB_H */

