/*
 * File: udi_time.h
 *
 * Public definitions for UDI time and timer management.
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

#ifndef _UDI_CORE_TIME_H
#define _UDI_CORE_TIME_H

#ifndef _UDI_H_INSIDE
#error "udi_time.h must not be #included directly; use udi.h instead."
#endif

/*
 * Timer Services
 */

typedef struct {
	udi_ubit32_t seconds;
	udi_ubit32_t nanoseconds;
} udi_time_t;

/*
 * Callback functions.
 */
typedef void udi_timer_expired_call_t(
		udi_cb_t *gcb);
typedef void udi_timer_tick_call_t(
		void *context,
		udi_ubit32_t nmissed);

/*
 * Function prototypes for timer services.
 */
void udi_timer_start(
		udi_timer_expired_call_t *callback,
		udi_cb_t *gcb,
		udi_time_t interval);
void udi_timer_start_repeating(
		udi_timer_tick_call_t *callback,
		udi_cb_t *gcb,
		udi_time_t interval);
void udi_timer_cancel(
		udi_cb_t *gcb);

/*
 * Function prototypes for timestamp services.
 */
udi_timestamp_t udi_time_current(void);
udi_time_t udi_time_between (
		udi_timestamp_t	start_time,
		udi_timestamp_t	end_time);
udi_time_t udi_time_since (
		udi_timestamp_t	start_time);

#endif /* _UDI_CORE_TIME_H */
