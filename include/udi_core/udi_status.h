/*
 * File: udi_status.h
 *
 * Public definitions for UDI status codes.
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

#ifndef _UDI_CORE_STATUS_H
#define _UDI_CORE_STATUS_H

#ifndef _UDI_H_INSIDE
#error "udi_status.h must not be #included directly; use udi.h instead."
#endif

/*
 * Status/error type.
 */
typedef udi_ubit32_t	udi_status_t;

#define UDI_STATUS_CODE_MASK		0x0000FFFF
#define UDI_SPECIFIC_STATUS_MASK 	0x00007FFF
#define UDI_STAT_META_SPECIFIC		0x00008000
#define UDI_CORRELATE_OFFSET		16
#define UDI_CORRELATE_MASK		0xFFFF0000
/*
 * Common Status Codes
 */
#define UDI_OK				0	/* Request completed properly */
#define UDI_STAT_NOT_SUPPORTED		1	/* Not yet supported */
#define UDI_STAT_NOT_UNDERSTOOD		2	/* Request not understood */
#define UDI_STAT_INVALID_STATE		3	/* Understood and implemented,
						   but inappropriate in current
						   state */
#define UDI_STAT_MISTAKEN_IDENTITY	4	/* Understood and implemented,
						   but inappropriate for this
						   device or other object */
#define UDI_STAT_ABORTED		5	/* Operation aborted by user */
#define UDI_STAT_TIMEOUT		6	/* Op exceeded time period */
#define UDI_STAT_BUSY			7	/* Resource busy */
#define UDI_STAT_RESOURCE_UNAVAIL	8	/* This resource unavailable */
#define UDI_STAT_HW_PROBLEM		9	/* Hardware problem */
#define UDI_STAT_NOT_RESPONDING		10	/* Device not responding */
#define UDI_STAT_DATA_UNDERRUN		11	/* Data underrun */
#define UDI_STAT_DATA_OVERRUN		12	/* Data overrun */
#define UDI_STAT_DATA_ERROR		13	/* Data corrupted */
#define UDI_STAT_PARENT_DRV_ERROR	14	/* Parent driver error */
#define UDI_STAT_CANNOT_BIND		15	/* Cannot bind to parent */
#define UDI_STAT_CANNOT_BIND_EXCL	16	/* Cannot bind exclusively 
						   to parent */
#define UDI_STAT_TOO_MANY_PARENTS	17	/* Too many parent for 
						   this driver */
#define UDI_STAT_BAD_PARENT_TYPE	18	/* Cannot bind to this type
						   of parent device */
#define UDI_STAT_TERMINATED		19	/* Region was abruptly 
						   terminated */
#define UDI_STAT_ATTR_MISMATCH		20	/* Driver/device cannot comply
						   with custom attribute 
						   setting */

#endif /* _UDI_CORE_STATUS_H */

