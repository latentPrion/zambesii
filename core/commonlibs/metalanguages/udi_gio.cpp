/*
 * File:  meta/udi_gio.c
 *
 * UDI Generic I/O Metalanguage Library
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

#define UDI_VERSION 0x101
#include <udi.h>		/* Public Core Environment Definitions */	
#include "udi_gioP.h"		/* Private GIO Meta Library Definitions */

/*
 * ========================================================================
 *
 * Ops Vector Templates
 *
 * ------------------------------------------------------------------------
 */

static udi_mei_ops_vec_template_t gio_ops_vec_template_list[] = {
	{
		UDI_GIO_PROVIDER_OPS_NUM,
		UDI_MEI_REL_BIND | UDI_MEI_REL_EXTERNAL | UDI_MEI_REL_INTERNAL,
		udi_gio_provider_op_templates,
	},
	{
		UDI_GIO_CLIENT_OPS_NUM,
		UDI_MEI_REL_BIND | UDI_MEI_REL_INITIATOR |
			UDI_MEI_REL_EXTERNAL | UDI_MEI_REL_INTERNAL,
		udi_gio_client_op_templates,
	},
	{
		0, 0, NULL /* Terminate list */
	}
};


udi_mei_init_t udi_gio_meta_info = {
	gio_ops_vec_template_list,
};


/*
 * ========================================================================
 *
 * Unmarshalling Routines (aka "Metalanguage Stubs")
 *
 * After dequeuing a control block from the region queue the environment
 * calls the corresponding metalanguage-specific unmarshalling routine to
 * unmarshal the extra parameters from the control block and make the
 * associated call to the driver.
 * ------------------------------------------------------------------------
 */


/*
 *  udi_gio_bind_req - Request a binding to a GIO provider driver
 */
UDI_MEI_STUBS(udi_gio_bind_req, udi_gio_bind_cb_t,
	      0, (), (), (),
	      UDI_GIO_PROVIDER_OPS_NUM, UDI_GIO_BIND_REQ)

/*
 *  udi_gio_bind_ack - Acknowledge a GIO binding
 */
UDI_MEI_STUBS(udi_gio_bind_ack, udi_gio_bind_cb_t,
	      3, (device_size_lo, device_size_hi, status),
		 (udi_ubit32_t, udi_ubit32_t, udi_status_t),
		 (UDI_VA_UBIT32_T, UDI_VA_UBIT32_T, UDI_VA_STATUS_T),
	      UDI_GIO_CLIENT_OPS_NUM, UDI_GIO_BIND_ACK)

/*
 *  udi_gio_unbind_req - Request to unbind from a GIO provider driver
 */
UDI_MEI_STUBS(udi_gio_unbind_req, udi_gio_bind_cb_t,
	      0, (), (), (),
	      UDI_GIO_PROVIDER_OPS_NUM, UDI_GIO_UNBIND_REQ)

/*
 *  udi_gio_unbind_ack - Acknowledge a GIO unbind request
 */
UDI_MEI_STUBS(udi_gio_unbind_ack, udi_gio_bind_cb_t,
	      0, (), (), (),
	      UDI_GIO_CLIENT_OPS_NUM, UDI_GIO_UNBIND_ACK)

/*
 *  udi_gio_xfer_req - Request a Generic IO transfer
 */
UDI_MEI_STUBS(udi_gio_xfer_req, udi_gio_xfer_cb_t,
	      0, (), (), (),
	      UDI_GIO_PROVIDER_OPS_NUM, UDI_GIO_XFER_REQ)

/*
 *  udi_gio_xfer_ack - Acknowledge a GIO transfer operation
 */
UDI_MEI_STUBS(udi_gio_xfer_ack, udi_gio_xfer_cb_t,
	      0, (), (), (),
	      UDI_GIO_CLIENT_OPS_NUM, UDI_GIO_XFER_ACK)

/*
 *  udi_gio_xfer_nak - Abnormal completion of a GIO transfer request
 */
UDI_MEI_STUBS(udi_gio_xfer_nak, udi_gio_xfer_cb_t,
	      1, (status), (udi_status_t), (UDI_VA_STATUS_T),
	      UDI_GIO_CLIENT_OPS_NUM, UDI_GIO_XFER_NAK)

/*
 *  udi_gio_event_ind - GIO event indication
 */
UDI_MEI_STUBS(udi_gio_event_ind, udi_gio_event_cb_t,
	      0, (), (), (),
	      UDI_GIO_CLIENT_OPS_NUM, UDI_GIO_EVENT_IND)

/*
 *  udi_gio_event_ind - GIO event response
 */
UDI_MEI_STUBS(udi_gio_event_res, udi_gio_event_cb_t,
	      0, (), (), (),
	      UDI_GIO_PROVIDER_OPS_NUM, UDI_GIO_EVENT_RES)

/*
 * "Proxy" Functions
 */
void
udi_gio_event_ind_unused (
	udi_gio_event_cb_t *gio_event_cb )
{
	/* Should never be called */
	udi_assert(0);
}

void
udi_gio_event_res_unused (
	udi_gio_event_cb_t *gio_event_cb )
{
	/* Should never be called */
	udi_assert(0);
}

