/*
 * File: udi_imc.h
 *
 * Public definitions for UDI Inter-Module Communication (IMC) services.
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

#ifndef _UDI_CORE_IMC_H
#define _UDI_CORE_IMC_H

#ifndef _UDI_H_INSIDE
#error "udi_imc.h must not be #included directly; use udi.h instead."
#endif

/*
 * Opaque types.
 */
#define UDI_NULL_CHANNEL ((udi_channel_t)NULL)

/*
 * Forward reference to struct __udi_mgmt_ops in meta/udi_mgmt.h
 */
typedef const struct __udi_mgmt_ops  udi_mgmt_ops_t;
struct __udi_mgmt_ops;

/*
 * Platform specific allocation and access limits
 */
typedef struct _udi_limits_t {
	udi_size_t max_legal_alloc;
	udi_size_t max_safe_alloc;
	udi_size_t max_trace_log_formatted_len;
	udi_size_t max_instance_attr_len;
	udi_ubit32_t min_curtime_res;
	udi_ubit32_t min_timer_res;
} udi_limits_t;

/* 
 * Primary region initialization structure
 */
typedef const struct _udi_primary_init_t {
	udi_mgmt_ops_t *mgmt_ops;
	const udi_ubit8_t *mgmt_op_flags;
	udi_size_t mgmt_scratch_requirement;
	udi_ubit8_t enumeration_attr_list_length;
	udi_size_t rdata_size;
	udi_size_t child_data_size;
	udi_ubit8_t per_parent_paths;
} udi_primary_init_t;

/*
 * Secondary region initialization structure
 */
typedef const struct _udi_secondary_init_t {
	udi_index_t region_idx;
	udi_size_t rdata_size;
} udi_secondary_init_t;


/*
 * Ops vector initialization structure
 */
typedef void udi_op_t(void);
typedef udi_op_t * const udi_ops_vector_t;
typedef const struct _udi_ops_init_t {
	udi_index_t ops_idx;
	udi_index_t meta_idx;
	udi_index_t meta_ops_num;
	udi_size_t chan_context_size;
	udi_ops_vector_t *ops_vector;
	const udi_ubit8_t *op_flags;
} udi_ops_init_t;

/*
 * Flags in udi_ops_init.op_flags .
 */
#define UDI_OP_LONG_EXEC	(1U<<0)

/*
 * Control block initialization structure
 */
typedef const struct _udi_cb_init_t {
	udi_index_t cb_idx;
	udi_index_t meta_idx;
	udi_index_t meta_cb_num;
	udi_size_t scratch_requirement;
	udi_size_t inline_size;
	udi_layout_t *inline_layout;
} udi_cb_init_t;

/*
 * Control block selections for incoming channel ops
 */
typedef const struct _udi_cb_select_t {
	udi_index_t ops_idx;
	udi_index_t cb_idx;
} udi_cb_select_t;

/*
 * Generic control block initialization properties
 */
typedef const struct _udi_gcb_init_t {
	udi_index_t cb_idx;
	udi_size_t scratch_requirement;
} udi_gcb_init_t;

/* 
 *  Module initialization structure
 */
typedef const struct _udi_init_t {
	udi_primary_init_t *primary_init_info;
	udi_secondary_init_t *secondary_init_list;
	udi_ops_init_t *ops_init_list;
	udi_cb_init_t *cb_init_list;
	udi_gcb_init_t *gcb_init_list;
	udi_cb_select_t *cb_select_list;
} udi_init_t;

#define UDI_MIN_ALLOC_LIMIT		4000
#define UDI_MIN_TRACE_LOG_LIMIT		200
#define UDI_MIN_INSTANCE_ATTR_LIMIT	64

/*
 * Initial context for new regions.
 */
typedef struct _udi_init_context_t {
	udi_index_t region_idx;
	udi_limits_t limits;
} udi_init_context_t;

/*
 * Initial context for bind channels
 */
typedef struct _udi_chan_context_t {
	void *rdata;
} udi_chan_context_t;

/*
 * Initial context for child-bind channels
 */
typedef struct _udi_child_chan_context_t {
	void *rdata;
	udi_ubit32_t	child_ID;
} udi_child_chan_context_t;

/*
 * Callback types.
 */
typedef void udi_channel_anchor_call_t(
		udi_cb_t *gcb,
		udi_channel_t anchored_channel);

typedef void udi_channel_spawn_call_t(
		udi_cb_t *gcb,
		udi_channel_t new_channel);

/*
 * Function prototypes for environment services.
 */
void udi_channel_anchor(
		udi_channel_anchor_call_t *callback,
		udi_cb_t *gcb,
		udi_channel_t channel,
		udi_index_t ops_idx,
		void *channel_context);

void udi_channel_spawn(
		udi_channel_spawn_call_t *callback,
		udi_cb_t *gcb,
		udi_channel_t channel,
		udi_index_t spawn_idx,
		udi_index_t ops_idx,
		void *channel_context);

void udi_channel_op_abort(
		udi_channel_t target_channel,
		udi_cb_t *orig_cb);

void udi_channel_set_context(
		udi_channel_t target_channel,
		void *channel_context);

void udi_channel_close(
		udi_channel_t channel);

#endif /* _UDI_CORE_IMC_H */

