/*
 * File: udi_mgmt.h
 *
 * UDI Management Metalanguage Public Definitions
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

#ifndef _UDI_CORE_META_MGMT_H
#define _UDI_CORE_META_MGMT_H

#ifndef _UDI_H_INSIDE
#error "udi_mgmt.h must not be #included directly; use udi.h instead."
#endif

/*
 * Control block groupings.
 * For mgmt meta, all control blocks are in a single group.
 */
#define UDI_MGMT_CB_NUM      		0

/*
 * Resource levels.
 */
#define UDI_RESOURCES_CRITICAL		1
#define UDI_RESOURCES_LOW		2
#define UDI_RESOURCES_NORMAL		3
#define UDI_RESOURCES_PLENTIFUL		4

/*
 * Device Management operation codes.
 */
#define UDI_DMGMT_PREPARE_TO_SUSPEND    1
#define UDI_DMGMT_SUSPEND		2
#define UDI_DMGMT_SHUTDOWN		3
#define UDI_DMGMT_PARENT_SUSPENDED	4
#define UDI_DMGMT_RESUME		5
#define UDI_DMGMT_UNBIND		6

/*
 * Device Management flags.
 */
#define UDI_DMGMT_NONTRANSPARENT	(1U<<0)

/*
 * Metalanguage-specific status code for Device Management.
 */
#define UDI_DMGMT_STAT_ROUTING_CHANGE	(UDI_STAT_META_SPECIFIC|1)

/*
 * Enumeration levels.
 */
#define UDI_ENUMERATE_START		1
#define UDI_ENUMERATE_START_RESCAN	2
#define UDI_ENUMERATE_NEXT		3
#define UDI_ENUMERATE_NEW		4
#define UDI_ENUMERATE_DIRECTED		5
#define UDI_ENUMERATE_RELEASE		6

/*
 * Enumeration result codes.
 */
#define UDI_ENUMERATE_OK		0
#define UDI_ENUMERATE_LEAF		1
#define UDI_ENUMERATE_DONE		2
#define UDI_ENUMERATE_RESCAN		3
#define UDI_ENUMERATE_REMOVED		4
#define UDI_ENUMERATE_REMOVED_SELF	5
#define UDI_ENUMERATE_RELEASED		6
#define UDI_ENUMERATE_FAILED		255

/*
 * Enumeration filter element structure.
 */
typedef struct {
	char attr_name[UDI_MAX_ATTR_NAMELEN];
	udi_ubit8_t attr_min[UDI_MAX_ATTR_SIZE];
	udi_ubit8_t attr_min_len;
	udi_ubit8_t attr_max[UDI_MAX_ATTR_SIZE];
	udi_ubit8_t attr_max_len;
	udi_instance_attr_type_t attr_type;
	udi_ubit32_t attr_stride;
} udi_filter_element_t;

/*
 * Control Blocks.
 */
typedef struct {
	udi_cb_t gcb;
	udi_index_t region_idx;
	udi_channel_t channel_to_secondary;
} udi_region_attach_cb_t;

typedef struct {
	udi_cb_t gcb;
	udi_channel_t bind_channel;
	udi_index_t meta_idx;
} udi_bind_cb_t;

typedef struct {
	udi_cb_t gcb;
	udi_ubit32_t child_ID;
	void *child_data;
	udi_instance_attr_list_t *attr_list;
	udi_ubit8_t attr_valid_length;
	const udi_filter_element_t *filter_list;
	udi_ubit8_t filter_list_length;
	udi_ubit8_t parent_ID;
} udi_enumerate_cb_t;

/* Special parent_ID filter values */
#define UDI_ANY_PARENT_ID		0

typedef struct {
	udi_cb_t gcb;
} udi_mgmt_cb_t;

typedef struct {
	udi_cb_t gcb;
	udi_trevent_t trace_mask;
	udi_index_t meta_idx;
} udi_usage_cb_t;

/*
 * Interface operations vectors.
 */
typedef void udi_usage_ind_op_t(
		udi_usage_cb_t *cb,
		udi_ubit8_t resource_level);
typedef void udi_enumerate_req_op_t(
		udi_enumerate_cb_t *cb,
		udi_ubit8_t enumeration_level);
typedef void udi_devmgmt_req_op_t(
		udi_mgmt_cb_t *cb,
		udi_ubit8_t mgmt_op,
		udi_ubit8_t parent_id);
typedef void udi_final_cleanup_req_op_t(
		udi_mgmt_cb_t *cb);

/*
 * Interface operations structure.
 */
struct __udi_mgmt_ops {
	udi_usage_ind_op_t * usage_ind_op;
	udi_enumerate_req_op_t * enumerate_req_op;
	udi_devmgmt_req_op_t * devmgmt_req_op;
	udi_final_cleanup_req_op_t * final_cleanup_req_op;
} ;

/*
 * Function prototypes.
 */
void udi_usage_res(
		udi_usage_cb_t *cb);
void udi_enumerate_ack(
		udi_enumerate_cb_t *cb,
		udi_ubit8_t enumeration_result,
		udi_index_t ops_idx);
void udi_devmgmt_ack(
		udi_mgmt_cb_t *cb,
		udi_ubit8_t flags,
		udi_status_t status);
void udi_final_cleanup_ack(
		udi_mgmt_cb_t *cb);

/*
 * Pre-defined ops functions.
 */
udi_usage_ind_op_t udi_static_usage;
udi_enumerate_req_op_t udi_enumerate_no_children;

/*
 * The following definitions are common to *all* metalanguage *except*
 * the Management Metalanguage. Even though they're not part of the
 * Management Metalanguage, this was the most convienent place to put them.
 */

/*
 * Channel event types
 */
#define	UDI_CHANNEL_CLOSED	0
#define UDI_CHANNEL_BOUND	1
#define UDI_CHANNEL_OP_ABORTED	2

/*
 * Channel event control block
 */
typedef struct {
	udi_cb_t gcb;
	udi_ubit8_t event;
	union {
		struct {
			udi_cb_t *bind_cb;
		} internal_bound;
		struct {
			udi_cb_t *bind_cb;
			udi_ubit8_t parent_ID;
			udi_buf_path_t *path_handles;
		} parent_bound;
		udi_cb_t *orig_cb;
	} params;
} udi_channel_event_cb_t;

/*
 * Channel event notification.
 */
void udi_channel_event_ind(
		udi_channel_event_cb_t *cb);
typedef void udi_channel_event_ind_op_t(
		udi_channel_event_cb_t *cb);
void udi_channel_event_complete(
		udi_channel_event_cb_t *cb,
		udi_status_t status);

#endif /* _UDI_CORE_META_MGMT_H */
