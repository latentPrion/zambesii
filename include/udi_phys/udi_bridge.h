/*
 * File: udi_bridge.h
 *
 * UDI Bus Bridge Metalanguage Public Definitions
 *
 * UDI drivers must not directly #include this file. It is included as
 * part of udi_physio.h.
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

#ifndef _UDI_PHYSIO_BRIDGE_H
#define _UDI_PHYSIO_BRIDGE_H

#ifndef _UDI_PHYSIO_H_INSIDE
#error "udi_bridge.h must not be #included directly; use udi_physio.h instead."
#endif

/* Interrupt Event Flags.  */
#define UDI_INTR_MASKING_NOT_REQUIRED	(1U<<0)
#define UDI_INTR_OVERRUN_OCCURRED	(1U<<1)
#define UDI_INTR_PREPROCESSED		(1U<<2)

/*
 * Control block definitions.
 */
typedef struct {
	udi_cb_t	gcb;
} udi_bus_bind_cb_t;

typedef struct {
	udi_cb_t	gcb;
	udi_index_t	interrupt_idx;
	udi_ubit8_t	min_event_pend;
	udi_pio_handle_t	preprocessing_handle;
} udi_intr_attach_cb_t;

typedef struct {
	udi_cb_t	gcb;
	udi_index_t	interrupt_idx;
} udi_intr_detach_cb_t;

typedef struct {
	udi_cb_t	gcb;
	udi_buf_t	*event_buf;
	udi_ubit16_t	intr_result;
} udi_intr_event_cb_t;

/* intr_result values */
#define UDI_INTR_UNCLAIMED		(1U<<0)
#define UDI_INTR_NO_EVENT		(1U<<1)

/* cb_group definitions */
#define UDI_BUS_BIND_CB_NUM             1
#define UDI_BUS_INTR_ATTACH_CB_NUM	2
#define UDI_BUS_INTR_DETACH_CB_NUM	3
#define UDI_BUS_INTR_EVENT_CB_NUM	4

/* ops_num definitions */
#define UDI_BUS_DEVICE_OPS_NUM 		1
#define UDI_BUS_BRIDGE_OPS_NUM		2
#define UDI_BUS_INTR_HANDLER_OPS_NUM	3
#define UDI_BUS_INTR_DISPATCH_OPS_NUM	4

/*
 * Interface operations vectors.
 */
typedef void udi_bus_bind_req_op_t(
		udi_bus_bind_cb_t *cb);
typedef void udi_bus_bind_ack_op_t(
		udi_bus_bind_cb_t *cb,
		udi_dma_constraints_t dma_constraints,
		udi_ubit8_t preferred_endianness,
		udi_status_t status);
typedef void udi_bus_unbind_req_op_t(
		udi_bus_bind_cb_t *cb);
typedef void udi_bus_unbind_ack_op_t(
		udi_bus_bind_cb_t *cb);
typedef void udi_intr_attach_req_op_t(
		udi_intr_attach_cb_t *cb);
typedef void udi_intr_attach_ack_op_t(
		udi_intr_attach_cb_t *cb,
		udi_status_t status);
typedef void udi_intr_detach_req_op_t(
		udi_intr_detach_cb_t *cb);
typedef void udi_intr_detach_ack_op_t(
		udi_intr_detach_cb_t *cb);
typedef void udi_intr_event_ind_op_t(
		udi_intr_event_cb_t *cb,
		udi_ubit8_t flags);
typedef void udi_intr_event_rdy_op_t(
		udi_intr_event_cb_t *cb);

/* Proxies */
udi_intr_attach_ack_op_t udi_intr_attach_ack_unused;
udi_intr_detach_ack_op_t udi_intr_detach_ack_unused;

typedef const struct {
		udi_channel_event_ind_op_t *channel_event_ind_op;
		udi_bus_bind_req_op_t *bus_bind_req_op;
		udi_bus_unbind_req_op_t *bus_unbind_req_op;
		udi_intr_attach_req_op_t *intr_attach_req_op;
		udi_intr_detach_req_op_t *intr_detach_req_op;
} udi_bus_bridge_ops_t;

typedef const struct {
		udi_channel_event_ind_op_t *channel_event_ind_op;
		udi_bus_bind_ack_op_t *bus_bind_ack_op;
		udi_bus_unbind_ack_op_t *bus_unbind_ack_op;
		udi_intr_attach_ack_op_t *intr_attach_ack_op;
		udi_intr_detach_ack_op_t *intr_detach_ack_op;
} udi_bus_device_ops_t;

typedef const struct {
		udi_channel_event_ind_op_t *channel_event_ind_op;
		udi_intr_event_ind_op_t *intr_event_ind_op;
} udi_intr_handler_ops_t;

typedef const struct {
		udi_channel_event_ind_op_t *channel_event_ind_op;
		udi_intr_event_rdy_op_t *intr_event_rdy_op;
} udi_intr_dispatcher_ops_t;

/*
 * Function prototypes for various channel operations
 */
udi_bus_bind_req_op_t		udi_bus_bind_req;
udi_bus_bind_ack_op_t		udi_bus_bind_ack;
udi_bus_unbind_req_op_t		udi_bus_unbind_req;
udi_bus_unbind_ack_op_t		udi_bus_unbind_ack;
udi_intr_attach_req_op_t	udi_intr_attach_req;
udi_intr_attach_ack_op_t	udi_intr_attach_ack;
udi_intr_detach_req_op_t	udi_intr_detach_req;
udi_intr_detach_ack_op_t	udi_intr_detach_ack;
udi_intr_event_ind_op_t		udi_intr_event_ind;
udi_intr_event_rdy_op_t		udi_intr_event_rdy;

#endif /* _UDI_PHYSIO_BRIDGE_H */
