/*
 * File:  meta/gio/udi_gioP.h
 *
 * UDI Generic I/O Metalanguage Library private header
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

/* 
 * ---------------------------------------------------------------------
 * Back-end stubs.
 * ---------------------------------------------------------------------
 */

/* Function prototypes for unmarshal ops. */
static udi_mei_direct_stub_t  udi_gio_bind_req_direct;
static udi_mei_backend_stub_t udi_gio_bind_req_backend;
static udi_mei_direct_stub_t  udi_gio_bind_ack_direct;
static udi_mei_backend_stub_t udi_gio_bind_ack_backend;
static udi_mei_direct_stub_t  udi_gio_unbind_req_direct;
static udi_mei_backend_stub_t udi_gio_unbind_req_backend;
static udi_mei_direct_stub_t  udi_gio_unbind_ack_direct;
static udi_mei_backend_stub_t udi_gio_unbind_ack_backend;
static udi_mei_direct_stub_t  udi_gio_xfer_req_direct;
static udi_mei_backend_stub_t udi_gio_xfer_req_backend;
static udi_mei_direct_stub_t  udi_gio_xfer_ack_direct;
static udi_mei_backend_stub_t udi_gio_xfer_ack_backend;
static udi_mei_direct_stub_t  udi_gio_xfer_nak_direct;
static udi_mei_backend_stub_t udi_gio_xfer_nak_backend;
static udi_mei_direct_stub_t  udi_gio_event_res_direct;
static udi_mei_backend_stub_t udi_gio_event_res_backend;
static udi_mei_direct_stub_t  udi_gio_event_ind_direct;
static udi_mei_backend_stub_t udi_gio_event_ind_backend;

/*
 * ---------------------------------------------------------------------
 * Op Templates and associated op_idx's.
 *
 * TODO: Fill in typespecs in op templates.
 * ---------------------------------------------------------------------
 */

/* Provider op_idx's */
#define UDI_GIO_BIND_REQ	1
#define UDI_GIO_UNBIND_REQ	2
#define UDI_GIO_XFER_REQ	3
#define UDI_GIO_EVENT_RES	4

/* Client op_idx's */
#define UDI_GIO_BIND_ACK	1
#define UDI_GIO_UNBIND_ACK	2
#define UDI_GIO_XFER_ACK	3
#define UDI_GIO_XFER_NAK	4
#define UDI_GIO_EVENT_IND	5

static udi_layout_t udi_gio_bind_visible_layout[] = {
/* BEGIN udi_xfer_constraints_t */
	UDI_DL_UBIT32_T,	/*udi_ubit32_t udi_xfer_max;*/
	UDI_DL_UBIT32_T,	/*udi_ubit32_t udi_xfer_typical;*/
	UDI_DL_UBIT32_T,	/*udi_ubit32_t udi_xfer_granularity;*/
	UDI_DL_BOOLEAN_T,	/*udi_boolean_t udi_xfer_one_piece;*/
	UDI_DL_BOOLEAN_T,	/*udi_boolean_t udi_xfer_exact_size;*/
	UDI_DL_BOOLEAN_T,	/*udi_boolean_t udi_xfer_no_reorder;*/
/* END udi_xfer_constraints_t */
	UDI_DL_END
};

static udi_layout_t udi_gio_xfer_visible_layout[] = {
	UDI_DL_UBIT8_T,		/* udi_gio_op_t */
	UDI_DL_INLINE_DRIVER_TYPED,	/* tr_params */
	UDI_DL_BUF,	/* preserve if UDI_GIO_DIR_WRITE flag set in op */
		offsetof(udi_gio_xfer_cb_t, op),
		UDI_GIO_DIR_WRITE,
		UDI_GIO_DIR_WRITE,
	UDI_DL_END
};

static udi_layout_t udi_gio_event_visible_layout[] = {
	UDI_DL_UBIT8_T,			/* event_code */
	UDI_DL_INLINE_DRIVER_TYPED,	/* event_params */
	UDI_DL_END
};

static udi_layout_t empty_marshal_layout[]  = {
	UDI_DL_END
};

/* Provider ops templates */
static udi_mei_op_template_t udi_gio_provider_op_templates[] = {
	{ "udi_gio_bind_req", UDI_MEI_OPCAT_REQ, 
		0, /* flags */
		UDI_GIO_BIND_CB_NUM,
		UDI_GIO_CLIENT_OPS_NUM,	/* completion_ops_num */
		UDI_GIO_BIND_ACK,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_gio_bind_req_direct, udi_gio_bind_req_backend, 
		udi_gio_bind_visible_layout, empty_marshal_layout },
	{ "udi_gio_unbind_req", UDI_MEI_OPCAT_REQ, 
		0, /* flags */
		UDI_GIO_BIND_CB_NUM,
		UDI_GIO_CLIENT_OPS_NUM,	/* completion_ops_num */
		UDI_GIO_UNBIND_ACK,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_gio_unbind_req_direct, udi_gio_unbind_req_backend, 
		udi_gio_bind_visible_layout, empty_marshal_layout },
	{ "udi_gio_xfer_req", UDI_MEI_OPCAT_REQ, 
		UDI_MEI_OP_ABORTABLE,
		UDI_GIO_XFER_CB_NUM,
		UDI_GIO_CLIENT_OPS_NUM,	/* completion_ops_num */
		UDI_GIO_XFER_ACK,	/* completion_vec_index */
		UDI_GIO_CLIENT_OPS_NUM,	/* exception_ops_num */
		UDI_GIO_XFER_NAK,	/* exception_vec_idx */
		udi_gio_xfer_req_direct, udi_gio_xfer_req_backend, 
		udi_gio_xfer_visible_layout, empty_marshal_layout },
	{ "udi_gio_event_res", UDI_MEI_OPCAT_RES, 
		0, /* flags */
		UDI_GIO_EVENT_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_gio_event_res_direct, udi_gio_event_res_backend, 
		udi_gio_event_visible_layout, empty_marshal_layout },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

static udi_layout_t udi_gio_bind_ack_marshal_layout[] = {
	UDI_DL_UBIT32_T, /* device size lo */
	UDI_DL_UBIT32_T, /* device size hi */
	UDI_DL_STATUS_T, /* status */
	UDI_DL_END
};

static udi_layout_t udi_gio_xfer_nak_marshal_layout[] =  {
	UDI_DL_STATUS_T,
	UDI_DL_END
};

/* Client ops templates */
static udi_mei_op_template_t udi_gio_client_op_templates[] = {
	{ "udi_gio_bind_ack", UDI_MEI_OPCAT_ACK, 0, UDI_GIO_BIND_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_gio_bind_ack_direct, udi_gio_bind_ack_backend, 
		udi_gio_bind_visible_layout, udi_gio_bind_ack_marshal_layout },
	{ "udi_gio_unbind_ack", UDI_MEI_OPCAT_ACK, 0, UDI_GIO_BIND_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_gio_unbind_ack_direct, udi_gio_unbind_ack_backend, 
		udi_gio_bind_visible_layout, empty_marshal_layout },
	{ "udi_gio_xfer_ack", UDI_MEI_OPCAT_ACK, 0, UDI_GIO_XFER_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_gio_xfer_ack_direct, udi_gio_xfer_ack_backend, 
		udi_gio_xfer_visible_layout, empty_marshal_layout },
	{ "udi_gio_xfer_nak", UDI_MEI_OPCAT_NAK, 0, UDI_GIO_XFER_CB_NUM,
		0,	/* completion_ops_num */
		0,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_gio_xfer_nak_direct, udi_gio_xfer_nak_backend, 
		udi_gio_xfer_visible_layout, udi_gio_xfer_nak_marshal_layout },
	{ "udi_gio_event_ind", UDI_MEI_OPCAT_IND, 0, UDI_GIO_EVENT_CB_NUM,
		UDI_GIO_PROVIDER_OPS_NUM,	/* completion_ops_num */
		UDI_GIO_EVENT_RES,	/* completion_vec_index */
		0,	/* exception_ops_num */
		0,	/* exception_vec_idx */
		udi_gio_event_ind_direct, udi_gio_event_ind_backend, 
		udi_gio_event_visible_layout, empty_marshal_layout },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

