/*
 * File: udi_constraints.h
 *
 * Public definitions for UDI Constraints Management.
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

#ifndef _UDI_CORE_CONSTRAINTS_H
#define _UDI_CORE_CONSTRAINTS_H

#ifndef _UDI_H_INSIDE
#error "udi_constraints.h must not be #included directly; use udi.h instead."
#endif

typedef	_UDI_HANDLE	udi_dma_handle_t;

#define	UDI_NULL_DMA_HANDLE	((udi_dma_handle_t)NULL)

typedef udi_ubit8_t udi_dma_constraints_attr_t;

/*
 * DMA constraints handle (opaque type).
 *
 * Zambesii expects that the object pointed to by a udi_dma_constraints_t is
 * a `zudi_dma_constraints_attr_spec_t`. See zbz_udi_mgmt.h.
 */
typedef struct zudi_dma_constraints_ zudi_dma_constraints_t;
typedef zudi_dma_constraints_t *	udi_dma_constraints_t;
#define UDI_NULL_DMA_CONSTRAINTS	((udi_dma_constraints_t)NULL)

/*
 * Flags for udi_dma_constraints_attr_set().
 */
#define UDI_DMA_CONSTRAINTS_COPY	(1U<<0)

typedef struct _udi_dma_constraints_attr_spec_t {
	udi_dma_constraints_attr_t attr_type;
	udi_ubit32_t attr_value;
} udi_dma_constraints_attr_spec_t;

/*
 * Callback types.
 */
typedef void udi_dma_constraints_attr_set_call_t(
			udi_cb_t *gcb,
			udi_dma_constraints_t new_constraints,
			udi_status_t status);

/*
 * Function prototypes for constraints management
 */
void udi_dma_constraints_attr_set(
			udi_dma_constraints_attr_set_call_t *callback,
			udi_cb_t *gcb,
			udi_dma_constraints_t src_constraints,
			const udi_dma_constraints_attr_spec_t *attr_list,
			udi_ubit16_t list_length,
			udi_ubit8_t flags);

void udi_dma_constraints_attr_reset(
			udi_dma_constraints_t constraints,
			udi_dma_constraints_attr_t attr);

void udi_dma_constraints_free(
			udi_dma_constraints_t constraints);

struct __udi_buf;

typedef struct _udi_xfer_constraints_t {
	udi_ubit32_t udi_xfer_max;
	udi_ubit32_t udi_xfer_typical;
	udi_ubit32_t udi_xfer_granularity;
	udi_boolean_t udi_xfer_one_piece;
	udi_boolean_t udi_xfer_exact_size;
	udi_boolean_t udi_xfer_no_reorder;
} udi_xfer_constraints_t;

#endif /* _UDI_CORE_CONSTRAINTS_H */

