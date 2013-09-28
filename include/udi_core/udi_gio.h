/*
 * File: udi_gio.h
 *
 * Public definitions for Generic I/O (GIO) metalanguage.
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

#ifndef _UDI_CORE_GIO_H
#define _UDI_CORE_GIO_H

#ifndef _UDI_H_INSIDE
#error "udi_gio.h must not be #included directly; use udi.h instead."
#endif

/*
 * Ops vector type numbers.
 */
#define	UDI_GIO_PROVIDER_OPS_NUM	1
#define	UDI_GIO_CLIENT_OPS_NUM		2

/*
 * Control block groupings.
 */
#define UDI_GIO_BIND_CB_NUM	1
#define UDI_GIO_XFER_CB_NUM	2
#define UDI_GIO_EVENT_CB_NUM	3

/*
 * Data type definitions
 */
typedef udi_ubit8_t udi_gio_op_t;

/*
 * Control block definitions.
 */
typedef struct {
	udi_cb_t gcb;
	udi_xfer_constraints_t xfer_constraints;
} udi_gio_bind_cb_t;

typedef struct {
	udi_cb_t gcb;
	udi_gio_op_t op;
	void *tr_params;
	udi_buf_t *data_buf;
} udi_gio_xfer_cb_t;

typedef struct {
	udi_ubit32_t offset_lo;
	udi_ubit32_t offset_hi;
} udi_gio_rw_params_t;

typedef struct {
	udi_ubit8_t test_num;
	udi_ubit8_t test_params_size;
} udi_gio_diag_params_t;

typedef struct {
	udi_cb_t gcb;
	udi_ubit8_t event_code;
	void *event_params;
} udi_gio_event_cb_t;


/*
 * Prototypes for "callee"-side interface operation entry points.
 */
typedef void udi_gio_bind_req_op_t(
		udi_gio_bind_cb_t *cb);
typedef void udi_gio_bind_ack_op_t(
		udi_gio_bind_cb_t *cb,
		udi_ubit32_t device_size_lo,
		udi_ubit32_t device_size_hi,
		udi_status_t status);
typedef void udi_gio_unbind_req_op_t(
		udi_gio_bind_cb_t *cb);
typedef void udi_gio_unbind_ack_op_t(
		udi_gio_bind_cb_t *cb);
typedef void udi_gio_xfer_req_op_t(
		udi_gio_xfer_cb_t *cb);
typedef void udi_gio_xfer_ack_op_t(
		udi_gio_xfer_cb_t *cb);
typedef void udi_gio_xfer_nak_op_t(
		udi_gio_xfer_cb_t *cb,
		udi_status_t status);
typedef void udi_gio_event_ind_op_t(
		udi_gio_event_cb_t *cb);
typedef void udi_gio_event_res_op_t(
		udi_gio_event_cb_t *cb);


/*
 * Interface operations vectors.
 */
typedef const struct {
	udi_channel_event_ind_op_t *channel_event_ind_op;
	udi_gio_bind_ack_op_t *gio_bind_ack_op;
	udi_gio_unbind_ack_op_t *gio_unbind_ack_op;
	udi_gio_xfer_ack_op_t *gio_xfer_ack_op;
	udi_gio_xfer_nak_op_t *gio_xfer_nak_op;
	udi_gio_event_ind_op_t *gio_event_ind_op;
} udi_gio_client_ops_t;

typedef const struct {
	udi_channel_event_ind_op_t *channel_event_ind_op;
	udi_gio_bind_req_op_t *gio_bind_req_op;
	udi_gio_unbind_req_op_t *gio_unbind_req_op;
	udi_gio_xfer_req_op_t *gio_xfer_req_op;
	udi_gio_event_res_op_t *gio_event_res_op;
} udi_gio_provider_ops_t;

/*
 * Interface operations ("caller"-side).
 */

void udi_gio_bind_req(
	udi_gio_bind_cb_t *cb);

void udi_gio_bind_ack(
	udi_gio_bind_cb_t *cb,
	udi_ubit32_t device_size_lo,
	udi_ubit32_t device_size_hi,
	udi_status_t status);

void udi_gio_unbind_req(
	udi_gio_bind_cb_t *cb);

void udi_gio_unbind_ack(
	udi_gio_bind_cb_t *cb);

void udi_gio_xfer_req(
	udi_gio_xfer_cb_t *cb);

void udi_gio_xfer_ack(
	udi_gio_xfer_cb_t *cb);

void udi_gio_xfer_nak(
	udi_gio_xfer_cb_t *cb,
	udi_status_t status);

void udi_gio_event_ind(
	udi_gio_event_cb_t *cb);

void udi_gio_event_res(
	udi_gio_event_cb_t *cb);

/*
 * Pre-defined ops functions.
 */
udi_gio_event_ind_op_t udi_gio_event_ind_unused;
udi_gio_event_res_op_t udi_gio_event_res_unused;

/*
 * Constants.
 */

#define UDI_GIO_MAX_PARAMS_SIZE	255

#define UDI_GIO_DIR_READ	(1U<<6)
#define UDI_GIO_DIR_WRITE	(1U<<7)
#define _UDI_GIO_DIR		(UDI_GIO_DIR_READ|UDI_GIO_DIR_WRITE)

#define UDI_GIO_OP_READ		UDI_GIO_DIR_READ
#define UDI_GIO_OP_WRITE	UDI_GIO_DIR_WRITE

#define UDI_GIO_OP_CUSTOM	16
#define UDI_GIO_OP_MAX		64

#define	UDI_GIO_OP_DIAG_ENABLE	1
#define	UDI_GIO_OP_DIAG_DISABLE	2
#define	UDI_GIO_OP_DIAG_RUN_TEST	(3 | UDI_GIO_DIR_READ)

#endif /* _UDI_CORE_GIO_H */
