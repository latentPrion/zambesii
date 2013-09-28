/*
 * File: udi_layout.h
 *
 * Public definitions for UDI layout specifiers.
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

#ifndef _UDI_CORE_LAYOUT_H
#define _UDI_CORE_LAYOUT_H

#ifndef _UDI_H_INSIDE
#error "udi_layout.h must not be #included directly; use udi.h instead."
#endif

/*
 * Layout specifier element type.
 */
typedef const udi_ubit8_t udi_layout_t;

/*
 * Specific-Length Element Type Codes
 */
#define UDI_DL_UBIT8_T 		1 /* udi_ubit8_t */
#define UDI_DL_SBIT8_T 		2 /* udi_sbit8_t */
#define UDI_DL_UBIT16_T 	3 /* udi_ubit16_t */
#define UDI_DL_SBIT16_T 	4 /* udi_sbit16_t */
#define UDI_DL_UBIT32_T 	5 /* udi_ubit32_t */
#define UDI_DL_SBIT32_T 	6 /* udi_sbit32_t */
#define UDI_DL_BOOLEAN_T 	7 /* udi_boolean_t */
#define UDI_DL_STATUS_T 	8 /* udi_status_t */

/*
 * Other Self-Contained Element Type Codes
 */
#define UDI_DL_INDEX_T 		20 /* udi_index_t */

/*
 * Opaque Handle Element Type Codes
 */
#define UDI_DL_CHANNEL_T	30 /* udi_channel_t */
#define UDI_DL_ORIGIN_T 	32 /* udi_origin_t */

/*
 * Indirect Element Type Codes
 */
#define UDI_DL_BUF 		40 /* udi_buf_t * */
#define UDI_DL_CB 		41 /* ptr to Meta-specific cb */
#define UDI_DL_INLINE_UNTYPED 	42
#define UDI_DL_INLINE_DRIVER_TYPED 43
#define UDI_DL_MOVABLE_UNTYPED	44

/*
 * Nested Element Type Codes
 */
#define UDI_DL_INLINE_TYPED 	50
#define UDI_DL_MOVABLE_TYPED	51
#define UDI_DL_ARRAY 		52

#define UDI_DL_END 		0 /* End of element */

#endif /* _UDI_CORE_LAYOUT_H */

