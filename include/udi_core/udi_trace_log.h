/*
 * File: udi_trace_log.h
 *
 * UDI Tracing & Logging Public Definitions
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

#ifndef _UDI_CORE_TRACE_LOG_H
#define _UDI_CORE_TRACE_LOG_H

#ifndef _UDI_H_INSIDE
#error "udi_trace_log.h must not be #included directly; use udi.h instead."
#endif

typedef udi_ubit32_t udi_trevent_t;

/*
 * Callback Types.
 */
typedef void udi_log_write_call_t(
		udi_cb_t *gcb,
		udi_status_t correlated_status);

/* 
 * Function Prototypes
 */

void udi_trace_write(
	udi_init_context_t *init_context,
	udi_trevent_t trace_event,
	udi_index_t meta_idx,
	udi_ubit32_t msgnum,
	...);
void udi_log_write(
	udi_log_write_call_t *callback,
	udi_cb_t *gcb,
	udi_trevent_t trace_event,
	udi_ubit8_t severity,
	udi_index_t meta_idx,
	udi_status_t original_status,
	udi_ubit32_t msgnum,
	...);

/* Common Trace Events */
#define UDI_TREVENT_LOCAL_PROC_ENTRY	(1U<<0)
#define UDI_TREVENT_LOCAL_PROC_EXIT 	(1U<<1)
#define UDI_TREVENT_EXTERNAL_ERROR 	(1U<<2)

/* Metalanguage Trace Events */
#define UDI_TREVENT_IO_SCHEDULED 	(1U<<6)
#define UDI_TREVENT_IO_COMPLETED 	(1U<<7)
#define UDI_TREVENT_META_SPECIFIC_1 	(1U<<11)
#define UDI_TREVENT_META_SPECIFIC_2 	(1U<<12)
#define UDI_TREVENT_META_SPECIFIC_3 	(1U<<13)
#define UDI_TREVENT_META_SPECIFIC_4 	(1U<<14)
#define UDI_TREVENT_META_SPECIFIC_5 	(1U<<15)

/* Driver-Specific Trace Events */
#define UDI_TREVENT_INTERNAL_1 		(1U<<16)
#define UDI_TREVENT_INTERNAL_2 		(1U<<17)
#define UDI_TREVENT_INTERNAL_3 		(1U<<18)
#define UDI_TREVENT_INTERNAL_4 		(1U<<19)
#define UDI_TREVENT_INTERNAL_5 		(1U<<20)
#define UDI_TREVENT_INTERNAL_6 		(1U<<21)
#define UDI_TREVENT_INTERNAL_7 		(1U<<22)
#define UDI_TREVENT_INTERNAL_8 		(1U<<23)
#define UDI_TREVENT_INTERNAL_9 		(1U<<24)
#define UDI_TREVENT_INTERNAL_10 	(1U<<25)
#define UDI_TREVENT_INTERNAL_11 	(1U<<26)
#define UDI_TREVENT_INTERNAL_12 	(1U<<27)
#define UDI_TREVENT_INTERNAL_13 	(1U<<28)
#define UDI_TREVENT_INTERNAL_14 	(1U<<29)
#define UDI_TREVENT_INTERNAL_15 	(1U<<30)

/* Logging Event */
#define UDI_TREVENT_LOG 		(1U<<31)

/* 
 * Log classes (severity level)
 */
#define UDI_LOG_DISASTER    1
#define UDI_LOG_ERROR       2
#define UDI_LOG_WARNING     3
#define UDI_LOG_INFORMATION 4

#endif /* _UDI_CORE_TRACE_LOG_H */
